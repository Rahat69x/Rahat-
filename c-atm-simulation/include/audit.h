/**
 * include/audit.h
 * Tamper-evident SHA-256 hash-chained audit log engine (F7).
 */

#ifndef AUDIT_H
#define AUDIT_H

#include "types.h"
#include <stdint.h>
#include <stdbool.h>

void audit_init(Bank *bank);
bool audit_record(Bank *bank, const char *data_dir, const char *actor,
                  const char *action, const char *target, const char *detail);

bool audit_verify(const char *data_dir, const uint8_t *expected_head,
                  uint64_t *out_broken_seq, char *out_reason, size_t reason_len);

#endif /* AUDIT_H */
