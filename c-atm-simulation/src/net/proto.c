/**
 * src/net/proto.c
 * Safe serialization and parsing for ATMP/1 text protocol (F8, C3).
 */

#include "proto.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

bool proto_parse_req(const char *line, ProtoReq *out_req) {
    if (!line || !out_req) return false;
    memset(out_req, 0, sizeof(ProtoReq));

    char buf[PROTO_MAX_LINE];
    snprintf(buf, sizeof(buf), "%s", line);
    char *eol = strpbrk(buf, "\r\n");
    if (eol) *eol = '\0';

    char *tag = strtok(buf, "\t");
    char *id_str = strtok(NULL, "\t");
    char *op = strtok(NULL, "\t");
    if (!tag || !id_str || !op || strcmp(tag, "REQ") != 0) return false;

    out_req->req_id = (uint64_t)strtoull(id_str, NULL, 10);
    snprintf(out_req->op, sizeof(out_req->op), "%s", op);

    char *a1 = strtok(NULL, "\t");
    char *a2 = strtok(NULL, "\t");
    char *a3 = strtok(NULL, "\t");
    char *a4 = strtok(NULL, "\t");

    if (a1) snprintf(out_req->arg1, sizeof(out_req->arg1), "%s", a1);
    if (a2) snprintf(out_req->arg2, sizeof(out_req->arg2), "%s", a2);
    if (a3) snprintf(out_req->arg3, sizeof(out_req->arg3), "%s", a3);
    if (a4) snprintf(out_req->arg4, sizeof(out_req->arg4), "%s", a4);

    return true;
}

bool proto_format_req(char *out, size_t out_len, uint64_t req_id, const char *op,
                      const char *arg1, const char *arg2, const char *arg3, const char *arg4) {
    if (!out || out_len == 0 || !op) return false;
    int n;
    if (arg4 && arg4[0]) {
        n = snprintf(out, out_len, "REQ\t%llu\t%s\t%s\t%s\t%s\t%s\n", (unsigned long long)req_id, op,
                     arg1 ? arg1 : "-", arg2 ? arg2 : "-", arg3 ? arg3 : "-", arg4);
    } else if (arg3 && arg3[0]) {
        n = snprintf(out, out_len, "REQ\t%llu\t%s\t%s\t%s\t%s\n", (unsigned long long)req_id, op,
                     arg1 ? arg1 : "-", arg2 ? arg2 : "-", arg3);
    } else if (arg2 && arg2[0]) {
        n = snprintf(out, out_len, "REQ\t%llu\t%s\t%s\t%s\n", (unsigned long long)req_id, op,
                     arg1 ? arg1 : "-", arg2);
    } else if (arg1 && arg1[0]) {
        n = snprintf(out, out_len, "REQ\t%llu\t%s\t%s\n", (unsigned long long)req_id, op, arg1);
    } else {
        n = snprintf(out, out_len, "REQ\t%llu\t%s\n", (unsigned long long)req_id, op);
    }
    return (n > 0 && (size_t)n < out_len);
}

bool proto_parse_res(const char *line, ProtoRes *out_res) {
    if (!line || !out_res) return false;
    memset(out_res, 0, sizeof(ProtoRes));

    char buf[PROTO_MAX_LINE];
    snprintf(buf, sizeof(buf), "%s", line);
    char *eol = strpbrk(buf, "\r\n");
    if (eol) *eol = '\0';

    char *tag = strtok(buf, "\t");
    char *id_str = strtok(NULL, "\t");
    char *code_str = strtok(NULL, "\t");
    if (!tag || !id_str || !code_str || strcmp(tag, "RES") != 0) return false;

    out_res->req_id = (uint64_t)strtoull(id_str, NULL, 10);
    out_res->code = atoi(code_str);

    char *rest = strtok(NULL, "");
    if (rest) {
        snprintf(out_res->payload, sizeof(out_res->payload), "%s", rest);
    }
    return true;
}

bool proto_format_res(char *out, size_t out_len, uint64_t req_id, int code, const char *payload) {
    if (!out || out_len == 0) return false;
    int n;
    if (payload && payload[0]) {
        n = snprintf(out, out_len, "RES\t%llu\t%d\t%s\n", (unsigned long long)req_id, code, payload);
    } else {
        n = snprintf(out, out_len, "RES\t%llu\t%d\n", (unsigned long long)req_id, code);
    }
    return (n > 0 && (size_t)n < out_len);
}
