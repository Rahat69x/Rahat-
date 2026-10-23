/**
 * src/security/audit.c
 * Tamper-evident append-only audit log with SHA-256 hash chaining (F7).
 */

#include "audit.h"
#include "sha256.h"
#include "clock.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(_WIN32) || defined(_WIN64)
#include <direct.h>
#define MAKE_DIR(d) _mkdir(d)
#else
#include <sys/stat.h>
#define MAKE_DIR(d) mkdir(d, 0755)
#endif

static void ensure_dir_exists(const char *dir) {
    if (dir && dir[0] != '\0') {
        MAKE_DIR(dir);
    }
}

void audit_init(Bank *bank) {
    if (!bank) return;
    memset(bank->audit_head_hash, 0, HASH_SIZE);
    bank->next_audit_seq = 0;
}

static void compute_audit_hash(const uint8_t prev_hash[HASH_SIZE], uint64_t seq,
                               time_t clk, const char *actor, const char *action,
                               const char *target, const char *detail,
                               uint8_t hash_out[HASH_SIZE]) {
    Sha256Ctx ctx;
    sha256_init(&ctx);
    sha256_update(&ctx, prev_hash, HASH_SIZE);
    sha256_update(&ctx, &seq, sizeof(seq));
    sha256_update(&ctx, &clk, sizeof(clk));
    sha256_update(&ctx, actor, strlen(actor));
    sha256_update(&ctx, action, strlen(action));
    sha256_update(&ctx, target, strlen(target));
    sha256_update(&ctx, detail, strlen(detail));
    sha256_final(&ctx, hash_out);
}

bool audit_record(Bank *bank, const char *data_dir, const char *actor,
                  const char *action, const char *target, const char *detail) {
    if (!bank) return false;
    const char *dir = data_dir ? data_dir : "data";
    ensure_dir_exists(dir);
    char log_path[256];
    snprintf(log_path, sizeof(log_path), "%s/audit.log", dir);

    FILE *f = fopen(log_path, "a");
    if (!f) return false;

    uint64_t seq = ++bank->next_audit_seq;
    time_t clk = clock_now();
    uint8_t entry_hash[HASH_SIZE];

    compute_audit_hash(bank->audit_head_hash, seq, clk, actor, action, target, detail, entry_hash);

    char prev_hex[65], hash_hex[65];
    sha256_to_hex(bank->audit_head_hash, prev_hex);
    sha256_to_hex(entry_hash, hash_hex);

    fprintf(f, "%llu|%ld|%s|%s|%s|%s|%s|%s\n",
            (unsigned long long)seq, (long)clk, actor, action, target, detail, prev_hex, hash_hex);
    fflush(f);
    fclose(f);

    memcpy(bank->audit_head_hash, entry_hash, HASH_SIZE);
    return true;
}

static bool parse_hex32(const char *hex, uint8_t out[HASH_SIZE]) {
    if (strlen(hex) != 64) return false;
    for (int i = 0; i < HASH_SIZE; ++i) {
        char byte_str[3] = {hex[i * 2], hex[i * 2 + 1], '\0'};
        out[i] = (uint8_t)strtoul(byte_str, NULL, 16);
    }
    return true;
}

bool audit_verify(const char *data_dir, const uint8_t *expected_head,
                  uint64_t *out_broken_seq, char *out_reason, size_t reason_len) {
    const char *dir = data_dir ? data_dir : "data";
    char log_path[256];
    snprintf(log_path, sizeof(log_path), "%s/audit.log", dir);

    FILE *f = fopen(log_path, "r");
    if (!f) {
        if (out_reason) snprintf(out_reason, reason_len, "Audit log file does not exist");
        return false;
    }

    uint8_t current_prev[HASH_SIZE] = {0};
    uint64_t expected_seq = 1;
    char line[512];
    uint8_t last_hash[HASH_SIZE] = {0};

    while (fgets(line, sizeof(line), f)) {
        line[strcspn(line, "\r\n")] = '\0';
        char *seq_s = strtok(line, "|");
        char *clk_s = strtok(NULL, "|");
        char *actor = strtok(NULL, "|");
        char *action = strtok(NULL, "|");
        char *target = strtok(NULL, "|");
        char *detail = strtok(NULL, "|");
        char *prev_hex = strtok(NULL, "|");
        char *hash_hex = strtok(NULL, "|");

        if (!seq_s || !clk_s || !actor || !action || !target || !detail || !prev_hex || !hash_hex) {
            if (out_broken_seq) *out_broken_seq = expected_seq;
            if (out_reason) snprintf(out_reason, reason_len, "Malformed audit line format at seq %llu", (unsigned long long)expected_seq);
            fclose(f);
            return false;
        }

        uint64_t seq = strtoull(seq_s, NULL, 10);
        time_t clk = (time_t)strtol(clk_s, NULL, 10);

        if (seq != expected_seq) {
            if (out_broken_seq) *out_broken_seq = seq;
            if (out_reason) snprintf(out_reason, reason_len, "Sequence gap detected. Expected %llu, found %llu",
                                     (unsigned long long)expected_seq, (unsigned long long)seq);
            fclose(f);
            return false;
        }

        uint8_t read_prev[HASH_SIZE], read_hash[HASH_SIZE], computed_hash[HASH_SIZE];
        parse_hex32(prev_hex, read_prev);
        parse_hex32(hash_hex, read_hash);

        if (memcmp(current_prev, read_prev, HASH_SIZE) != 0) {
            if (out_broken_seq) *out_broken_seq = seq;
            if (out_reason) snprintf(out_reason, reason_len, "Broken hash chain at seq %llu: prev_hash mismatch", (unsigned long long)seq);
            fclose(f);
            return false;
        }

        compute_audit_hash(current_prev, seq, clk, actor, action, target, detail, computed_hash);
        if (memcmp(computed_hash, read_hash, HASH_SIZE) != 0) {
            if (out_broken_seq) *out_broken_seq = seq;
            if (out_reason) snprintf(out_reason, reason_len, "Payload tamper at seq %llu: hash mismatch", (unsigned long long)seq);
            fclose(f);
            return false;
        }

        memcpy(current_prev, read_hash, HASH_SIZE);
        memcpy(last_hash, read_hash, HASH_SIZE);
        expected_seq++;
    }
    fclose(f);

    if (expected_head && memcmp(last_hash, expected_head, HASH_SIZE) != 0) {
        if (out_broken_seq) *out_broken_seq = expected_seq - 1;
        if (out_reason) snprintf(out_reason, reason_len, "Log head does not match stored head hash in bank.dat");
        return false;
    }

    return true;
}
