/**
 * include/server.h
 * Single-threaded event-driven bank daemon server (F8, Section 5).
 */

#ifndef SERVER_H
#define SERVER_H

#include "types.h"
#include "proto.h"
#include "net/net_compat.h"
#include <stdbool.h>

#define MAX_SERVER_CLIENTS 16
#define TOKEN_HEX_LEN 32

typedef struct {
    socket_t sock;
    bool in_use;
    bool handshaked;
    bool authenticated;
    long account_id;
    char token[TOKEN_HEX_LEN + 1];
    uint8_t login_nonce[16];
    time_t last_active;
    
    /* Request read buffer */
    char in_buf[PROTO_MAX_LINE];
    size_t in_len;

    /* Idempotency cache */
    uint64_t last_req_id;
    char last_response[PROTO_MAX_LINE];
    bool has_last_response;
} ClientSession;

typedef struct {
    Bank *bank;
    socket_t listen_sock;
    char bind_addr[64];
    int port;
    int max_clients;
    ClientSession clients[MAX_SERVER_CLIENTS];
} BankServer;

bool server_init(BankServer *srv, Bank *bank, const char *bind_addr, int port, int max_clients);

void server_run(BankServer *srv, volatile int *stop_flag);

void server_close(BankServer *srv);

#endif /* SERVER_H */
