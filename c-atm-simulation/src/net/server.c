/**
 * src/net/server.c
 * Single-threaded event-driven bank daemon with ATMP/1 protocol and challenge-response auth (F8, C8).
 */

#include "server.h"
#include "account.h"
#include "txn.h"
#include "pin.h"
#include "hmac.h"
#include "rng.h"
#include "otp.h"
#include "fraud.h"
#include "audit.h"
#include "store.h"
#include "clock.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static void hex_encode(const uint8_t *in, size_t len, char *out) {
    static const char hex[] = "0123456789abcdef";
    for (size_t i = 0; i < len; ++i) {
        out[i * 2] = hex[(in[i] >> 4) & 0x0F];
        out[i * 2 + 1] = hex[in[i] & 0x0F];
    }
    out[len * 2] = '\0';
}

static bool hex_decode(const char *in, uint8_t *out, size_t out_len) {
    if (!in || strlen(in) != out_len * 2) return false;
    for (size_t i = 0; i < out_len; ++i) {
        unsigned int byte_val = 0;
        if (sscanf(in + (i * 2), "%02x", &byte_val) != 1) return false;
        out[i] = (uint8_t)byte_val;
    }
    return true;
}

static bool send_all(socket_t s, const char *buf, size_t len) {
    size_t total = 0;
    while (total < len) {
        int n = send(s, buf + total, (int)(len - total), 0);
        if (n <= 0) return false;
        total += (size_t)n;
    }
    return true;
}

static void send_reply(ClientSession *cs, uint64_t req_id, int code, const char *payload) {
    char res_line[PROTO_MAX_LINE];
    proto_format_res(res_line, sizeof(res_line), req_id, code, payload);
    send_all(cs->sock, res_line, strlen(res_line));

    cs->last_req_id = req_id;
    snprintf(cs->last_response, sizeof(cs->last_response), "%s", res_line);
    cs->has_last_response = true;
}

static void handle_challenge(BankServer *srv, ClientSession *cs, const ProtoReq *req) {
    long acct = atol(req->arg1);
    Account *acc = account_find(srv->bank, acct);
    if (!acc) {
        send_reply(cs, req->req_id, 404, "Account not found");
        return;
    }
    if (acc->status == STATUS_BLOCKED) {
        send_reply(cs, req->req_id, 403, "Account is BLOCKED");
        return;
    }
    rng_bytes(cs->login_nonce, 16);
    char salt_hex[33], nonce_hex[33];
    hex_encode(acc->pin_salt, 16, salt_hex);
    hex_encode(cs->login_nonce, 16, nonce_hex);

    char payload[128];
    snprintf(payload, sizeof(payload), "%s\t%d\t%s", salt_hex, acc->pin_iter, nonce_hex);
    cs->account_id = acct;
    send_reply(cs, req->req_id, 200, payload);
}

static void handle_login(BankServer *srv, ClientSession *cs, const ProtoReq *req) {
    long acct = atol(req->arg1);
    Account *acc = account_find(srv->bank, acct);
    if (!acc || acct != cs->account_id) {
        send_reply(cs, req->req_id, 404, "Invalid challenge sequence");
        return;
    }
    uint8_t client_hmac[32], expected_hmac[32];
    if (!hex_decode(req->arg2, client_hmac, 32)) {
        send_reply(cs, req->req_id, 400, "Malformed HMAC hex");
        return;
    }
    hmac_sha256(acc->pin_hash, 32, cs->login_nonce, 16, expected_hmac);

    if (pin_const_time_memcmp(client_hmac, expected_hmac, 32) != 0) {
        acc->failed_pins++;
        fraud_record_failure(srv->bank, acc->id, "Remote challenge login failure");
        if (acc->failed_pins >= srv->bank->config.max_pin_tries) {
            acc->status = STATUS_BLOCKED;
        }
        send_reply(cs, req->req_id, 401, "Invalid PIN response");
        return;
    }

    acc->failed_pins = 0;
    uint8_t raw_token[16];
    rng_bytes(raw_token, 16);
    hex_encode(raw_token, 16, cs->token);
    cs->authenticated = true;

    char payload[128];
    snprintf(payload, sizeof(payload), "%s\t%s\t%d", cs->token, acc->holder_name, acc->must_change_pin ? 1 : 0);
    send_reply(cs, req->req_id, 200, payload);
}

static void handle_withdraw(BankServer *srv, ClientSession *cs, const ProtoReq *req) {
    Account *acc = account_find(srv->bank, cs->account_id);
    int64_t amt = (int64_t)atoll(req->arg2);
    int64_t mult_minor = srv->bank->config.withdraw_multiple * 100;
    int64_t per_lim = (acc->per_withdraw_override > 0) ? acc->per_withdraw_override : (srv->bank->config.per_withdraw_limit * 100);

    if (amt <= 0 || (amt % mult_minor) != 0) {
        send_reply(cs, req->req_id, 400, "Withdrawal must be positive multiple of 100");
        return;
    }
    if (amt > per_lim || amt > (acc->balance)) {
        send_reply(cs, req->req_id, 400, "Limit exceeded or insufficient funds");
        return;
    }

    acc->balance -= amt;
    acc->withdrawn_today += amt;
    char receipt[RECEIPT_ID_LEN + 1];
    txn_record(srv->bank, acc->id, TXN_WITHDRAW, amt, acc->balance, 0, "Remote ATM withdrawal", receipt);
    store_save_bank("data", srv->bank);

    char payload[96];
    snprintf(payload, sizeof(payload), "%lld\t%s", (long long)acc->balance, receipt);
    send_reply(cs, req->req_id, 200, payload);
}

static void handle_deposit(BankServer *srv, ClientSession *cs, const ProtoReq *req) {
    Account *acc = account_find(srv->bank, cs->account_id);
    int64_t amt = (int64_t)atoll(req->arg2);
    if (amt <= 0) {
        send_reply(cs, req->req_id, 400, "Deposit amount must be positive");
        return;
    }
    acc->balance += amt;
    char receipt[RECEIPT_ID_LEN + 1];
    txn_record(srv->bank, acc->id, TXN_DEPOSIT, amt, acc->balance, 0, "Remote ATM deposit", receipt);
    store_save_bank("data", srv->bank);

    char payload[96];
    snprintf(payload, sizeof(payload), "%lld\t%s", (long long)acc->balance, receipt);
    send_reply(cs, req->req_id, 200, payload);
}

static void handle_balance(BankServer *srv, ClientSession *cs, const ProtoReq *req) {
    Account *acc = account_find(srv->bank, cs->account_id);
    char payload[48];
    snprintf(payload, sizeof(payload), "%lld", (long long)acc->balance);
    send_reply(cs, req->req_id, 200, payload);
}

static void dispatch_client_cmd(BankServer *srv, ClientSession *cs, const ProtoReq *req) {
    /* Idempotency check: replay identical response if duplicate req_id */
    if (cs->has_last_response && cs->last_req_id == req->req_id) {
        send_all(cs->sock, cs->last_response, strlen(cs->last_response));
        return;
    }

    if (strcmp(req->op, "CHALLENGE") == 0) {
        handle_challenge(srv, cs, req);
    } else if (strcmp(req->op, "LOGIN") == 0) {
        handle_login(srv, cs, req);
    } else {
        if (!cs->authenticated || strcmp(req->arg1, cs->token) != 0) {
            send_reply(cs, req->req_id, 401, "Unauthorized or invalid session token");
            return;
        }
        if (strcmp(req->op, "BALANCE") == 0) handle_balance(srv, cs, req);
        else if (strcmp(req->op, "DEPOSIT") == 0) handle_deposit(srv, cs, req);
        else if (strcmp(req->op, "WITHDRAW") == 0) handle_withdraw(srv, cs, req);
        else if (strcmp(req->op, "LOGOUT") == 0) {
            cs->authenticated = false;
            send_reply(cs, req->req_id, 200, "LOGGED_OUT");
        } else {
            send_reply(cs, req->req_id, 405, "Unsupported operation");
        }
    }
}

static void process_client_data(BankServer *srv, ClientSession *cs) {
    char raw[256];
    int n = recv(cs->sock, raw, sizeof(raw) - 1, 0);
    if (n <= 0) {
        CLOSESOCKET(cs->sock);
        cs->in_use = false;
        return;
    }
    raw[n] = '\0';
    cs->last_active = clock_now();

    for (int i = 0; i < n; ++i) {
        if (raw[i] == '\n') {
            cs->in_buf[cs->in_len] = '\0';
            if (!cs->handshaked) {
                if (strncmp(cs->in_buf, "HELLO\tATMP/1", 12) == 0) {
                    cs->handshaked = true;
                    send_all(cs->sock, PROTO_HANDSHAKE_OK, strlen(PROTO_HANDSHAKE_OK));
                } else {
                    CLOSESOCKET(cs->sock);
                    cs->in_use = false;
                    return;
                }
            } else {
                ProtoReq req;
                if (proto_parse_req(cs->in_buf, &req)) {
                    dispatch_client_cmd(srv, cs, &req);
                }
            }
            cs->in_len = 0;
        } else if (cs->in_len < sizeof(cs->in_buf) - 1) {
            cs->in_buf[cs->in_len++] = raw[i];
        }
    }
}

bool server_init(BankServer *srv, Bank *bank, const char *bind_addr, int port, int max_clients) {
    if (!srv || !bank) return false;
    net_init();
    memset(srv, 0, sizeof(BankServer));
    srv->bank = bank;
    snprintf(srv->bind_addr, sizeof(srv->bind_addr), "%s", bind_addr ? bind_addr : "127.0.0.1");
    srv->port = (port > 0) ? port : 5050;
    srv->max_clients = (max_clients > 0 && max_clients <= MAX_SERVER_CLIENTS) ? max_clients : 8;

    srv->listen_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (srv->listen_sock == INVALID_SOCKET_VAL) return false;

    int opt = 1;
    setsockopt(srv->listen_sock, SOL_SOCKET, SO_REUSEADDR, (const char *)&opt, sizeof(opt));

    struct sockaddr_in sin;
    memset(&sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_port = htons((uint16_t)srv->port);
    sin.sin_addr.s_addr = inet_addr(srv->bind_addr);

    if (bind(srv->listen_sock, (struct sockaddr *)&sin, sizeof(sin)) == SOCKET_ERR_VAL) {
        CLOSESOCKET(srv->listen_sock);
        srv->listen_sock = INVALID_SOCKET_VAL;
        return false;
    }
    if (listen(srv->listen_sock, 8) == SOCKET_ERR_VAL) {
        CLOSESOCKET(srv->listen_sock);
        srv->listen_sock = INVALID_SOCKET_VAL;
        return false;
    }
    return true;
}

void server_run(BankServer *srv, volatile int *stop_flag) {
    if (!srv || srv->listen_sock == INVALID_SOCKET_VAL) return;

    while (stop_flag && !(*stop_flag)) {
        fd_set read_fds;
        FD_ZERO(&read_fds);
        FD_SET(srv->listen_sock, &read_fds);
        socket_t max_fd = srv->listen_sock;

        for (int i = 0; i < srv->max_clients; ++i) {
            if (srv->clients[i].in_use) {
                FD_SET(srv->clients[i].sock, &read_fds);
                if (srv->clients[i].sock > max_fd) max_fd = srv->clients[i].sock;
            }
        }

        struct timeval tv = {1, 0}; /* 1s timeout */
        int activity = select((int)(max_fd + 1), &read_fds, NULL, NULL, &tv);
        if (activity < 0) break;

        /* Accept incoming connections */
        if (FD_ISSET(srv->listen_sock, &read_fds)) {
            struct sockaddr_in cin;
            int clen = sizeof(cin);
            socket_t csock = accept(srv->listen_sock, (struct sockaddr *)&cin, &clen);
            if (csock != INVALID_SOCKET_VAL) {
                int slot = -1;
                for (int i = 0; i < srv->max_clients; ++i) {
                    if (!srv->clients[i].in_use) { slot = i; break; }
                }
                if (slot >= 0) {
                    ClientSession *cs = &srv->clients[slot];
                    memset(cs, 0, sizeof(ClientSession));
                    cs->sock = csock;
                    cs->in_use = true;
                    cs->last_active = clock_now();
                } else {
                    CLOSESOCKET(csock);
                }
            }
        }

        /* Check clients */
        for (int i = 0; i < srv->max_clients; ++i) {
            ClientSession *cs = &srv->clients[i];
            if (cs->in_use) {
                if (FD_ISSET(cs->sock, &read_fds)) {
                    process_client_data(srv, cs);
                } else if (clock_now() - cs->last_active > srv->bank->config.session_idle_s) {
                    CLOSESOCKET(cs->sock);
                    cs->in_use = false;
                }
            }
        }
    }
}

void server_close(BankServer *srv) {
    if (!srv) return;
    for (int i = 0; i < srv->max_clients; ++i) {
        if (srv->clients[i].in_use) {
            CLOSESOCKET(srv->clients[i].sock);
            srv->clients[i].in_use = false;
        }
    }
    if (srv->listen_sock != INVALID_SOCKET_VAL) {
        CLOSESOCKET(srv->listen_sock);
        srv->listen_sock = INVALID_SOCKET_VAL;
    }
    net_cleanup();
}
