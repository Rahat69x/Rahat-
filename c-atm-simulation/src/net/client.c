/**
 * src/net/client.c
 * Remote BankApi implementation over TCP sockets and ATMP/1 protocol (F8, C3, C8).
 */

#include "client.h"
#include "proto.h"
#include "pin.h"
#include "hmac.h"
#include "net/net_compat.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

typedef struct {
    char host[64];
    int port;
    socket_t sock;
    bool connected;
    uint64_t next_req_id;
    char token[64];
    long current_account_id;
} RemoteClientCtx;

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

static bool client_send_recv(RemoteClientCtx *ctx, const char *req_str, ProtoRes *out_res) {
    if (!ctx || ctx->sock == INVALID_SOCKET_VAL) return false;
    size_t len = strlen(req_str);
    size_t sent = 0;
    while (sent < len) {
        int n = send(ctx->sock, req_str + sent, (int)(len - sent), 0);
        if (n <= 0) return false;
        sent += (size_t)n;
    }

    char res_line[PROTO_MAX_LINE];
    size_t rlen = 0;
    while (rlen < sizeof(res_line) - 1) {
        char ch;
        int n = recv(ctx->sock, &ch, 1, 0);
        if (n <= 0) return false;
        res_line[rlen++] = ch;
        if (ch == '\n') break;
    }
    res_line[rlen] = '\0';
    return proto_parse_res(res_line, out_res);
}

static bool ensure_connected(RemoteClientCtx *ctx) {
    if (ctx->connected) return true;
    net_init();
    ctx->sock = socket(AF_INET, SOCK_STREAM, 0);
    if (ctx->sock == INVALID_SOCKET_VAL) return false;

    struct sockaddr_in sin;
    memset(&sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_port = htons((uint16_t)ctx->port);
    sin.sin_addr.s_addr = inet_addr(ctx->host);

    if (connect(ctx->sock, (struct sockaddr *)&sin, sizeof(sin)) == SOCKET_ERR_VAL) {
        CLOSESOCKET(ctx->sock);
        ctx->sock = INVALID_SOCKET_VAL;
        return false;
    }

    /* Handshake */
    send(ctx->sock, PROTO_HANDSHAKE, (int)strlen(PROTO_HANDSHAKE), 0);
    char buf[64];
    int n = recv(ctx->sock, buf, sizeof(buf) - 1, 0);
    if (n <= 0 || strncmp(buf, "HELLO\tATMP/1\tOK", 15) != 0) {
        CLOSESOCKET(ctx->sock);
        ctx->sock = INVALID_SOCKET_VAL;
        return false;
    }
    ctx->connected = true;
    return true;
}

static ApiResult remote_login(void *c, long account_id, const char *pin, char out_name[MAX_NAME_LEN + 1]) {
    RemoteClientCtx *ctx = (RemoteClientCtx *)c;
    ApiResult res = {false, 0, "", "", 0};
    if (!ensure_connected(ctx)) {
        res.error_code = 503;
        snprintf(res.message, sizeof(res.message), "Connection to bank server failed");
        return res;
    }

    /* 1. Request Challenge */
    char req_str[PROTO_MAX_LINE];
    char acct_str[32];
    snprintf(acct_str, sizeof(acct_str), "%ld", account_id);
    proto_format_req(req_str, sizeof(req_str), ++ctx->next_req_id, "CHALLENGE", acct_str, NULL, NULL, NULL);

    ProtoRes ch_res;
    if (!client_send_recv(ctx, req_str, &ch_res) || ch_res.code != 200) {
        res.error_code = ch_res.code ? ch_res.code : 500;
        snprintf(res.message, sizeof(res.message), "%.127s", ch_res.payload[0] ? ch_res.payload : "Challenge failed");
        return res;
    }

    /* Parse challenge response: salt, iter, nonce */
    char salt_hex[64], nonce_hex[64];
    int iter = 0;
    if (sscanf(ch_res.payload, "%63s\t%d\t%63s", salt_hex, &iter, nonce_hex) != 3) {
        res.error_code = 500;
        snprintf(res.message, sizeof(res.message), "Malformed challenge from server");
        return res;
    }

    /* 2. Compute PIN hash and HMAC response locally */
    uint8_t salt[16], nonce[16], pin_hash[32], client_hmac[32];
    hex_decode(salt_hex, salt, 16);
    hex_decode(nonce_hex, nonce, 16);
    pin_hash_derive(pin, salt, iter, pin_hash);
    hmac_sha256(pin_hash, 32, nonce, 16, client_hmac);

    char hmac_hex[65];
    hex_encode(client_hmac, 32, hmac_hex);

    /* 3. Send LOGIN with HMAC */
    proto_format_req(req_str, sizeof(req_str), ++ctx->next_req_id, "LOGIN", acct_str, hmac_hex, NULL, NULL);
    ProtoRes login_res;
    if (!client_send_recv(ctx, req_str, &login_res) || login_res.code != 200) {
        res.error_code = login_res.code ? login_res.code : 401;
        snprintf(res.message, sizeof(res.message), "%.127s", login_res.payload[0] ? login_res.payload : "Login failed");
        return res;
    }

    int must_change = 0;
    sscanf(login_res.payload, "%63s\t%48[^\t]\t%d", ctx->token, out_name, &must_change);
    ctx->current_account_id = account_id;
    res.success = true;
    res.error_code = 0;
    snprintf(res.message, sizeof(res.message), "Login successful");
    return res;
}

static ApiResult remote_get_balance(void *c, long account_id, int64_t *out_balance) {
    (void)account_id;
    RemoteClientCtx *ctx = (RemoteClientCtx *)c;
    ApiResult res = {false, 0, "", "", 0};
    char req[PROTO_MAX_LINE];
    proto_format_req(req, sizeof(req), ++ctx->next_req_id, "BALANCE", ctx->token, NULL, NULL, NULL);

    ProtoRes r;
    if (client_send_recv(ctx, req, &r) && r.code == 200) {
        res.success = true;
        res.balance_after = (int64_t)atoll(r.payload);
        if (out_balance) *out_balance = res.balance_after;
    } else {
        res.error_code = r.code;
        snprintf(res.message, sizeof(res.message), "%.127s", r.payload);
    }
    return res;
}

static ApiResult remote_withdraw(void *c, long account_id, int64_t amount) {
    (void)account_id;
    RemoteClientCtx *ctx = (RemoteClientCtx *)c;
    ApiResult res = {false, 0, "", "", 0};
    char req[PROTO_MAX_LINE], amt_str[32];
    snprintf(amt_str, sizeof(amt_str), "%lld", (long long)amount);
    proto_format_req(req, sizeof(req), ++ctx->next_req_id, "WITHDRAW", ctx->token, amt_str, NULL, NULL);

    ProtoRes r;
    if (client_send_recv(ctx, req, &r) && r.code == 200) {
        res.success = true;
        int64_t bal = 0;
        char rec[RECEIPT_ID_LEN + 1] = {0};
        sscanf(r.payload, "%lld\t%24s", (long long *)&bal, rec);
        res.balance_after = bal;
        snprintf(res.receipt_no, sizeof(res.receipt_no), "%s", rec);
    } else {
        res.error_code = r.code;
        snprintf(res.message, sizeof(res.message), "%.127s", r.payload);
    }
    return res;
}

static ApiResult remote_deposit(void *c, long account_id, int64_t amount) {
    (void)account_id;
    RemoteClientCtx *ctx = (RemoteClientCtx *)c;
    ApiResult res = {false, 0, "", "", 0};
    char req[PROTO_MAX_LINE], amt_str[32];
    snprintf(amt_str, sizeof(amt_str), "%lld", (long long)amount);
    proto_format_req(req, sizeof(req), ++ctx->next_req_id, "DEPOSIT", ctx->token, amt_str, NULL, NULL);

    ProtoRes r;
    if (client_send_recv(ctx, req, &r) && r.code == 200) {
        res.success = true;
        int64_t bal = 0;
        char rec[RECEIPT_ID_LEN + 1] = {0};
        sscanf(r.payload, "%lld\t%24s", (long long *)&bal, rec);
        res.balance_after = bal;
        snprintf(res.receipt_no, sizeof(res.receipt_no), "%s", rec);
    } else {
        res.error_code = r.code;
        snprintf(res.message, sizeof(res.message), "%.127s", r.payload);
    }
    return res;
}

static ApiResult remote_transfer(void *c, long src_id, long dst_id, int64_t amount, const char *otp) {
    (void)src_id; (void)dst_id; (void)amount; (void)otp; (void)c;
    ApiResult res = {false, 501, "Not implemented remotely", "", 0};
    return res;
}

static ApiResult remote_get_history(void *c, long account_id, Txn *out_txns, size_t max_count, size_t *out_count) {
    (void)c; (void)account_id; (void)out_txns; (void)max_count;
    if (out_count) *out_count = 0;
    ApiResult res = {true, 0, "No remote history implemented", "", 0};
    return res;
}

static ApiResult remote_change_pin(void *c, long account_id, const char *old_pin, const char *new_pin) {
    (void)c; (void)account_id; (void)old_pin; (void)new_pin;
    ApiResult res = {false, 501, "Not implemented remotely", "", 0};
    return res;
}

static ApiResult remote_logout(void *c, long account_id) {
    (void)account_id;
    RemoteClientCtx *ctx = (RemoteClientCtx *)c;
    ApiResult res = {true, 0, "Logged out", "", 0};
    char req[PROTO_MAX_LINE];
    proto_format_req(req, sizeof(req), ++ctx->next_req_id, "LOGOUT", ctx->token, NULL, NULL, NULL);
    ProtoRes r;
    client_send_recv(ctx, req, &r);
    CLOSESOCKET(ctx->sock);
    ctx->sock = INVALID_SOCKET_VAL;
    ctx->connected = false;
    return res;
}

BankApi bank_api_create_remote(const char *host, int port) {
    RemoteClientCtx *ctx = malloc(sizeof(RemoteClientCtx));
    if (!ctx) {
        BankApi dummy = {NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL};
        return dummy;
    }
    memset(ctx, 0, sizeof(RemoteClientCtx));
    snprintf(ctx->host, sizeof(ctx->host), "%s", host ? host : "127.0.0.1");
    ctx->port = (port > 0) ? port : 5050;
    ctx->sock = INVALID_SOCKET_VAL;
    ctx->connected = false;

    BankApi api;
    api.ctx = ctx;
    api.login = remote_login;
    api.get_balance = remote_get_balance;
    api.deposit = remote_deposit;
    api.withdraw = remote_withdraw;
    api.transfer = remote_transfer;
    api.get_history = remote_get_history;
    api.change_pin = remote_change_pin;
    api.logout = remote_logout;
    return api;
}
