/**
 * src/storage/store.c
 * Endian-safe atomic binary persistence with CRC-32 validation and lock files (F4).
 */

#include "store.h"
#include "crc32.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(_WIN32) || defined(_WIN64)
#include <windows.h>
#include <io.h>
#define F_OK 0
#else
#include <unistd.h>
#include <fcntl.h>
#endif

/* Serialization helpers (little-endian) */
static void write_u8(FILE *f, uint8_t v) { fputc(v, f); }
static void write_u32(FILE *f, uint32_t v) {
    uint8_t b[4] = {(uint8_t)(v & 0xFF), (uint8_t)((v >> 8) & 0xFF),
                    (uint8_t)((v >> 16) & 0xFF), (uint8_t)((v >> 24) & 0xFF)};
    fwrite(b, 1, 4, f);
}
static void write_u64(FILE *f, uint64_t v) {
    uint8_t b[8];
    for (int i = 0; i < 8; ++i) b[i] = (uint8_t)((v >> (i * 8)) & 0xFF);
    fwrite(b, 1, 8, f);
}
static void write_i64(FILE *f, int64_t v) { write_u64(f, (uint64_t)v); }
static void write_bytes(FILE *f, const void *data, size_t len) { fwrite(data, 1, len, f); }

static bool read_u8(FILE *f, uint8_t *v) {
    int ch = fgetc(f);
    if (ch == EOF) return false;
    *v = (uint8_t)ch;
    return true;
}
static bool read_u32(FILE *f, uint32_t *v) {
    uint8_t b[4];
    if (fread(b, 1, 4, f) != 4) return false;
    *v = ((uint32_t)b[0]) | (((uint32_t)b[1]) << 8) |
         (((uint32_t)b[2]) << 16) | (((uint32_t)b[3]) << 24);
    return true;
}
static bool read_u64(FILE *f, uint64_t *v) {
    uint8_t b[8];
    if (fread(b, 1, 8, f) != 8) return false;
    uint64_t res = 0;
    for (int i = 0; i < 8; ++i) res |= (((uint64_t)b[i]) << (i * 8));
    *v = res;
    return true;
}
static bool read_i64(FILE *f, int64_t *v) {
    uint64_t u;
    if (!read_u64(f, &u)) return false;
    *v = (int64_t)u;
    return true;
}
static bool read_bytes(FILE *f, void *data, size_t len) {
    return fread(data, 1, len, f) == len;
}

bool store_acquire_lock(const char *data_dir, char *err_msg, size_t err_len) {
    char lock_path[256];
    snprintf(lock_path, sizeof(lock_path), "%s/.lock", data_dir ? data_dir : "data");

#if defined(_WIN32) || defined(_WIN64)
    HANDLE hFile = CreateFileA(lock_path, GENERIC_WRITE, 0, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
        if (err_msg && err_len > 0) {
            snprintf(err_msg, err_len, "Lock file exists at '%s'. If no other instance is running, delete it.", lock_path);
        }
        return false;
    }
    CloseHandle(hFile);
    return true;
#else
    int fd = open(lock_path, O_WRONLY | O_CREAT | O_EXCL, 0600);
    if (fd < 0) {
        if (err_msg && err_len > 0) {
            snprintf(err_msg, err_len, "Lock file exists at '%s'. Remove it if stale.", lock_path);
        }
        return false;
    }
    close(fd);
    return true;
#endif
}

void store_release_lock(const char *data_dir) {
    char lock_path[256];
    snprintf(lock_path, sizeof(lock_path), "%s/.lock", data_dir ? data_dir : "data");
    remove(lock_path);
}

void store_migrate_v1_to_v2(Bank *bank) {
    /* Migration stub: ensures all accounts have non-zero PIN iterations */
    for (size_t i = 0; i < bank->account_count; ++i) {
        if (bank->accounts[i].pin_iter <= 0) {
            bank->accounts[i].pin_iter = 10000;
        }
    }
}

static bool serialize_bank(FILE *f, const Bank *bank) {
    write_u32(f, (uint32_t)bank->sim_day);
    write_u64(f, bank->next_txn_id);
    write_u64(f, bank->next_audit_seq);
    write_bytes(f, bank->audit_head_hash, HASH_SIZE);

    /* Accounts */
    write_u32(f, (uint32_t)bank->account_count);
    for (size_t i = 0; i < bank->account_count; ++i) {
        const Account *a = &bank->accounts[i];
        write_i64(f, a->id);
        write_bytes(f, a->holder_name, sizeof(a->holder_name));
        write_bytes(f, a->pin_salt, SALT_SIZE);
        write_bytes(f, a->pin_hash, HASH_SIZE);
        write_u32(f, (uint32_t)a->pin_iter);
        write_u8(f, (uint8_t)a->must_change_pin);
        write_i64(f, a->balance);
        write_i64(f, a->withdrawn_today);
        write_u32(f, (uint32_t)a->last_withdraw_day);
        write_u32(f, (uint32_t)a->status);
        write_u32(f, (uint32_t)a->failed_pins);
        write_u32(f, (uint32_t)a->risk_score);
        write_i64(f, a->interest_remainder);
        write_u32(f, (uint32_t)a->created_day);
        write_i64(f, a->per_withdraw_override);
        write_i64(f, a->daily_withdraw_override);
    }

    /* Admins */
    write_u32(f, (uint32_t)bank->admin_count);
    for (size_t i = 0; i < bank->admin_count; ++i) {
        const Admin *adm = &bank->admins[i];
        write_bytes(f, adm->id, sizeof(adm->id));
        write_bytes(f, adm->name, sizeof(adm->name));
        write_bytes(f, adm->passphrase_salt, SALT_SIZE);
        write_bytes(f, adm->passphrase_hash, HASH_SIZE);
        write_u32(f, (uint32_t)adm->role);
        write_u8(f, (uint8_t)adm->is_locked);
        write_u32(f, (uint32_t)adm->failed_attempts);
        write_u8(f, (uint8_t)adm->must_change_pass);
    }
    return true;
}

static bool deserialize_bank(FILE *f, Bank *bank) {
    uint32_t sim_day_u, acc_count, adm_count;
    if (!read_u32(f, &sim_day_u) || !read_u64(f, &bank->next_txn_id) ||
        !read_u64(f, &bank->next_audit_seq) ||
        !read_bytes(f, bank->audit_head_hash, HASH_SIZE)) {
        return false;
    }
    bank->sim_day = (int)sim_day_u;

    if (!read_u32(f, &acc_count) || acc_count > MAX_ACCOUNTS) return false;
    bank->account_count = acc_count;

    for (size_t i = 0; i < acc_count; ++i) {
        Account *a = &bank->accounts[i];
        int64_t acc_id;
        uint32_t iter, lw_day, st, fails, risk, c_day;
        uint8_t must_change;

        if (!read_i64(f, &acc_id) || !read_bytes(f, a->holder_name, sizeof(a->holder_name)) ||
            !read_bytes(f, a->pin_salt, SALT_SIZE) || !read_bytes(f, a->pin_hash, HASH_SIZE) ||
            !read_u32(f, &iter) || !read_u8(f, &must_change) ||
            !read_i64(f, &a->balance) || !read_i64(f, &a->withdrawn_today) ||
            !read_u32(f, &lw_day) || !read_u32(f, &st) ||
            !read_u32(f, &fails) || !read_u32(f, &risk) ||
            !read_i64(f, &a->interest_remainder) || !read_u32(f, &c_day) ||
            !read_i64(f, &a->per_withdraw_override) || !read_i64(f, &a->daily_withdraw_override)) {
            return false;
        }
        a->id = (long)acc_id;
        a->pin_iter = (int)iter;
        a->must_change_pin = (must_change != 0);
        a->last_withdraw_day = (int)lw_day;
        a->status = (AccountStatus)st;
        a->failed_pins = (int)fails;
        a->risk_score = (int)risk;
        a->created_day = (int)c_day;
    }

    if (!read_u32(f, &adm_count) || adm_count > MAX_ADMINS) return false;
    bank->admin_count = adm_count;
    for (size_t i = 0; i < adm_count; ++i) {
        Admin *adm = &bank->admins[i];
        uint32_t r, fails;
        uint8_t locked, must_change;
        if (!read_bytes(f, adm->id, sizeof(adm->id)) || !read_bytes(f, adm->name, sizeof(adm->name)) ||
            !read_bytes(f, adm->passphrase_salt, SALT_SIZE) || !read_bytes(f, adm->passphrase_hash, HASH_SIZE) ||
            !read_u32(f, &r) || !read_u8(f, &locked) || !read_u32(f, &fails) || !read_u8(f, &must_change)) {
            return false;
        }
        adm->role = (AdminRole)r;
        adm->is_locked = (locked != 0);
        adm->failed_attempts = (int)fails;
        adm->must_change_pass = (must_change != 0);
    }
    return true;
}

bool store_save_bank(const char *data_dir, const Bank *bank) {
    if (!bank) return false;
    const char *dir = data_dir ? data_dir : "data";
    char tmp_path[256], dat_path[256], bak_path[256];
    snprintf(tmp_path, sizeof(tmp_path), "%s/bank.dat.tmp", dir);
    snprintf(dat_path, sizeof(dat_path), "%s/bank.dat", dir);
    snprintf(bak_path, sizeof(bak_path), "%s/bank.dat.bak", dir);

    FILE *f = fopen(tmp_path, "w+b");
    if (!f) return false;

    /* Header placeholder: 4 magic + 4 version + 4 crc32 */
    write_bytes(f, STORE_MAGIC, 4);
    write_u32(f, STORE_FORMAT_VERSION);
    write_u32(f, 0); /* CRC placeholder */

    long payload_start = ftell(f);
    serialize_bank(f, bank);
    long payload_end = ftell(f);

    /* Compute CRC-32 over payload */
    fflush(f);
    fseek(f, payload_start, SEEK_SET);
    size_t payload_len = (size_t)(payload_end - payload_start);
    uint8_t *buf = malloc(payload_len);
    if (!buf) { fclose(f); remove(tmp_path); return false; }
    if (fread(buf, 1, payload_len, f) != payload_len) {
        free(buf); fclose(f); remove(tmp_path); return false;
    }
    uint32_t crc = crc32_calculate(buf, payload_len);
    free(buf);

    /* Write CRC back to header */
    fseek(f, 8, SEEK_SET);
    write_u32(f, crc);
    fflush(f);
    fclose(f);

    /* Safe atomic rotation: dat -> bak, tmp -> dat */
    remove(bak_path);
    rename(dat_path, bak_path);
    remove(dat_path);
    if (rename(tmp_path, dat_path) != 0) {
        /* Rollback if rename fails */
        rename(bak_path, dat_path);
        return false;
    }
    return true;
}

static bool try_load_file(const char *path, Bank *bank) {
    FILE *f = fopen(path, "rb");
    if (!f) return false;

    char magic[5] = {0};
    uint32_t ver = 0, stored_crc = 0;
    if (!read_bytes(f, magic, 4) || !read_u32(f, &ver) || !read_u32(f, &stored_crc)) {
        fclose(f);
        return false;
    }
    if (memcmp(magic, STORE_MAGIC, 4) != 0) {
        fclose(f);
        return false;
    }

    long payload_start = ftell(f);
    fseek(f, 0, SEEK_END);
    long payload_end = ftell(f);
    fseek(f, payload_start, SEEK_SET);

    size_t payload_len = (size_t)(payload_end - payload_start);
    uint8_t *buf = malloc(payload_len);
    if (!buf) { fclose(f); return false; }
    if (fread(buf, 1, payload_len, f) != payload_len) {
        free(buf); fclose(f); return false;
    }
    uint32_t calc_crc = crc32_calculate(buf, payload_len);
    free(buf);

    if (calc_crc != stored_crc) {
        fclose(f);
        return false;
    }

    fseek(f, payload_start, SEEK_SET);
    bool ok = deserialize_bank(f, bank);
    fclose(f);
    if (ok && ver < STORE_FORMAT_VERSION) {
        store_migrate_v1_to_v2(bank);
    }
    return ok;
}

bool store_load_bank(const char *data_dir, Bank *bank) {
    if (!bank) return false;
    const char *dir = data_dir ? data_dir : "data";
    char dat_path[256], bak_path[256];
    snprintf(dat_path, sizeof(dat_path), "%s/bank.dat", dir);
    snprintf(bak_path, sizeof(bak_path), "%s/bank.dat.bak", dir);

    if (try_load_file(dat_path, bank)) {
        return true;
    }
    /* Fallback to backup */
    if (try_load_file(bak_path, bank)) {
        printf("[WARN] Primary database corrupted; restored from backup '%s'.\n", bak_path);
        return true;
    }
    return false;
}
