/**
 * include/proto.h
 * Text-based, tab-separated client-server protocol (F8, Section 5).
 * Format: REQ <req_id> <OP> <args...> \n
 *         RES <req_id> <code> <payload> \n
 */

#ifndef PROTO_H
#define PROTO_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#define PROTO_MAX_LINE 512
#define PROTO_HANDSHAKE "HELLO\tATMP/1\n"
#define PROTO_HANDSHAKE_OK "HELLO\tATMP/1\tOK\n"

typedef struct {
    uint64_t req_id;
    char op[24];
    char arg1[128];
    char arg2[128];
    char arg3[128];
    char arg4[128];
} ProtoReq;

typedef struct {
    uint64_t req_id;
    int code;
    char payload[PROTO_MAX_LINE - 48];
} ProtoRes;

bool proto_parse_req(const char *line, ProtoReq *out_req);
bool proto_format_req(char *out_buf, size_t out_len, uint64_t req_id, const char *op,
                      const char *arg1, const char *arg2, const char *arg3, const char *arg4);

bool proto_parse_res(const char *line, ProtoRes *out_res);
bool proto_format_res(char *out_buf, size_t out_len, uint64_t req_id, int code, const char *payload);

#endif /* PROTO_H */
