/**
 * include/store.h
 * Atomic persistence engine with CRC-32 integrity and single-writer locks (F4).
 */

#ifndef STORE_H
#define STORE_H

#include "types.h"
#include <stdbool.h>

#define STORE_MAGIC "ATMC"
#define STORE_FORMAT_VERSION 2U

bool store_acquire_lock(const char *data_dir, char *err_msg, size_t err_len);
void store_release_lock(const char *data_dir);

bool store_save_bank(const char *data_dir, const Bank *bank);
bool store_load_bank(const char *data_dir, Bank *bank);

bool store_save_txns(const char *data_dir, const Bank *bank);
bool store_load_txns(const char *data_dir, Bank *bank);

void store_migrate_v1_to_v2(Bank *bank);

#endif /* STORE_H */
