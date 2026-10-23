/**
 * @file atm.c
 * @brief Implementation of the Console-based ATM Simulation (C11 Standard)
 * Implements Core Features and Advanced Features A1 through A22.
 */

#include "atm.h"

/* Global Dynamic Accounts Array (managed through malloc / realloc / free) */
static Account *g_accounts = NULL;
static int g_account_count = 0;
static int g_account_capacity = 0;

/* Global transaction sequence counter for reference generation */
static uint32_t g_txn_seq = 1000U;

/* ========================================================================== */
/*                      SECURITY & CRYPTOGRAPHY (Core, A21)                   */
/* ========================================================================== */

/**
 * @brief Generates a random 16-character hexadecimal salt string.
 * @param salt Buffer of at least MAX_SALT_LEN bytes.
 * @param length Desired hex length (typically 16).
 */
void generate_salt(char *salt, size_t length) {
    static const char hex_digits[] = "0123456789abcdef";
    if (!salt || length == 0) return;
    for (size_t i = 0; i < length && i < MAX_SALT_LEN - 1; i++) {
        salt[i] = hex_digits[rand() % 16];
    }
    salt[length < MAX_SALT_LEN ? length : MAX_SALT_LEN - 1] = '\0';
}

/**
 * @brief Computes a salted, stretched hash for a 4-digit PIN.
 * Educational Note (A21): A 4-digit PIN has only 10,000 combinations (~13.3 bits
 * of entropy). Fast iterative hashing like this simulation provides basic salt
 * separation, but production banking software mandates cryptographic hardware
 * security modules (HSMs) and memory-hard key derivation like Argon2id or PBKDF2.
 */
void compute_pin_hash(const char *pin, const char *salt, char *output_hash) {
    if (!pin || !salt || !output_hash) return;

    /* 64-bit FNV-1a basis with 5,000 iterative stretching rounds */
    uint64_t hash = 0xcbf29ce484222325ULL;
    const uint64_t prime = 0x100000001b3ULL;

    for (int round = 0; round < 5000; round++) {
        /* Mix salt */
        for (const char *p = salt; *p; p++) {
            hash ^= (uint64_t)(unsigned char)(*p);
            hash *= prime;
        }
        /* Mix PIN */
        for (const char *p = pin; *p; p++) {
            hash ^= (uint64_t)(unsigned char)(*p);
            hash *= prime;
        }
        /* Add round index perturbation */
        hash ^= (uint64_t)round;
        hash *= prime;
    }

    /* Convert 64-bit stretched hash + salt fold into a 64-character hex string */
    uint64_t hash2 = hash ^ 0x9e3779b97f4a7c15ULL;
    for (const char *p = salt; *p; p++) {
        hash2 = (hash2 << 5) | (hash2 >> 59);
        hash2 ^= (uint64_t)(unsigned char)(*p);
    }

    snprintf(output_hash, MAX_HASH_LEN, "%016llx%016llx%016llx%016llx",
             (unsigned long long)hash,
             (unsigned long long)hash2,
             (unsigned long long)(hash ^ hash2),
             (unsigned long long)(hash + hash2));
}

/**
 * @brief Verifies whether an entered PIN matches the salted stored hash.
 */
bool verify_pin(const char *input_pin, const char *salt, const char *stored_hash) {
    if (!input_pin || !salt || !stored_hash) return false;
    char computed[MAX_HASH_LEN];
    compute_pin_hash(input_pin, salt, computed);
    return (strcmp(computed, stored_hash) == 0);
}

/**
 * @brief Generates a unique, chronological transaction reference ID (A12).
 * Format: TXN-YYYYMMDD-HHMMSS-XXXX
 */
void generate_reference_id(char *buffer, size_t size) {
    if (!buffer || size < MAX_REF_LEN) return;
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    g_txn_seq++;
    if (g_txn_seq > 9999U) g_txn_seq = 1000U;

    if (t) {
        snprintf(buffer, size, "TXN-%04d%02d%02d-%02d%02d%02d-%04u",
                 t->tm_year + 1900, t->tm_mon + 1, t->tm_mday,
                 t->tm_hour, t->tm_min, t->tm_sec, g_txn_seq);
    } else {
        snprintf(buffer, size, "TXN-00000000-000000-%04u", g_txn_seq);
    }
}

/* ========================================================================== */
/*                     CONFIGURATION & RATES (A6, A19)                        */
/* ========================================================================== */

/**
 * @brief Populates default banking configuration parameters.
 */
void load_default_config(BankConfig *config) {
    if (!config) return;
    config->session_timeout_sec = 60;
    config->max_pin_attempts = 3;
    config->otp_threshold = 10000.0;
    config->otp_expiry_sec = 60;
    config->savings_min_balance = 500.0;
    config->current_min_balance = 1000.0;
    config->student_min_balance = 100.0;
    config->savings_daily_limit = 20000.0;
    config->current_daily_limit = 100000.0;
    config->student_daily_limit = 10000.0;
    config->transfer_fee_percent = 0.50;
    config->free_withdrawals_per_month = 4;
    config->extra_withdrawal_fee = 15.0;
    config->savings_annual_interest = 4.50;
    config->current_annual_interest = 0.00;
    config->student_annual_interest = 2.50;
    config->fraud_velocity_count = 3;
    config->fraud_velocity_seconds = 120;
    config->fraud_transfer_pct = 0.80;
    config->low_notes_threshold = 20;

    generate_salt(config->admin_pin_salt, 16);
    compute_pin_hash(DEFAULT_ADMIN_PIN, config->admin_pin_salt, config->admin_pin_hash);
}

/**
 * @brief Loads and parses bank_config.txt with strict bounds validation.
 */
bool load_config_file(const char *filename, BankConfig *config) {
    if (!config) return false;
    load_default_config(config);

    FILE *fp = fopen(filename, "r");
    if (!fp) {
        /* File missing, write defaults */
        save_config_file(filename, config);
        return true;
    }

    char line[256];
    while (fgets(line, sizeof(line), fp)) {
        char *p = line;
        while (*p == ' ' || *p == '\t') p++;
        if (*p == '#' || *p == '\n' || *p == '\r' || *p == '\0') continue;

        char key[64], val[128];
        if (sscanf(p, "%63[^=]=%127s", key, val) == 2) {
            /* Trim trailing newline from val */
            val[strcspn(val, "\r\n")] = '\0';

            if (strcmp(key, "SESSION_TIMEOUT_SEC") == 0) {
                int v = (int)strtol(val, NULL, 10);
                if (v >= 10 && v <= 600) config->session_timeout_sec = v;
            } else if (strcmp(key, "MAX_PIN_ATTEMPTS") == 0) {
                int v = (int)strtol(val, NULL, 10);
                if (v >= 1 && v <= 10) config->max_pin_attempts = v;
            } else if (strcmp(key, "OTP_THRESHOLD") == 0) {
                double v = strtod(val, NULL);
                if (v >= 100.0) config->otp_threshold = v;
            } else if (strcmp(key, "OTP_EXPIRY_SEC") == 0) {
                int v = (int)strtol(val, NULL, 10);
                if (v >= 15 && v <= 300) config->otp_expiry_sec = v;
            } else if (strcmp(key, "SAVINGS_MIN_BALANCE") == 0) {
                config->savings_min_balance = strtod(val, NULL);
            } else if (strcmp(key, "CURRENT_MIN_BALANCE") == 0) {
                config->current_min_balance = strtod(val, NULL);
            } else if (strcmp(key, "STUDENT_MIN_BALANCE") == 0) {
                config->student_min_balance = strtod(val, NULL);
            } else if (strcmp(key, "SAVINGS_DAILY_LIMIT") == 0) {
                config->savings_daily_limit = strtod(val, NULL);
            } else if (strcmp(key, "CURRENT_DAILY_LIMIT") == 0) {
                config->current_daily_limit = strtod(val, NULL);
            } else if (strcmp(key, "STUDENT_DAILY_LIMIT") == 0) {
                config->student_daily_limit = strtod(val, NULL);
            } else if (strcmp(key, "TRANSFER_FEE_PERCENT") == 0) {
                config->transfer_fee_percent = strtod(val, NULL);
            } else if (strcmp(key, "FREE_WITHDRAWALS_PER_MONTH") == 0) {
                config->free_withdrawals_per_month = (int)strtol(val, NULL, 10);
            } else if (strcmp(key, "EXTRA_WITHDRAWAL_FEE") == 0) {
                config->extra_withdrawal_fee = strtod(val, NULL);
            } else if (strcmp(key, "SAVINGS_ANNUAL_INTEREST") == 0) {
                config->savings_annual_interest = strtod(val, NULL);
            } else if (strcmp(key, "CURRENT_ANNUAL_INTEREST") == 0) {
                config->current_annual_interest = strtod(val, NULL);
            } else if (strcmp(key, "STUDENT_ANNUAL_INTEREST") == 0) {
                config->student_annual_interest = strtod(val, NULL);
            } else if (strcmp(key, "FRAUD_VELOCITY_COUNT") == 0) {
                config->fraud_velocity_count = (int)strtol(val, NULL, 10);
            } else if (strcmp(key, "FRAUD_VELOCITY_SECONDS") == 0) {
                config->fraud_velocity_seconds = (int)strtol(val, NULL, 10);
            } else if (strcmp(key, "FRAUD_TRANSFER_PCT") == 0) {
                config->fraud_transfer_pct = strtod(val, NULL);
            } else if (strcmp(key, "LOW_NOTES_THRESHOLD") == 0) {
                config->low_notes_threshold = (int)strtol(val, NULL, 10);
            }
        }
    }
    fclose(fp);
    return true;
}

/**
 * @brief Saves bank_config.txt file.
 */
bool save_config_file(const char *filename, const BankConfig *config) {
    if (!config) return false;
    FILE *fp = fopen(filename, "w");
    if (!fp) return false;

    fprintf(fp, "# Bank Configuration Parameters\n");
    fprintf(fp, "SESSION_TIMEOUT_SEC=%d\n", config->session_timeout_sec);
    fprintf(fp, "MAX_PIN_ATTEMPTS=%d\n", config->max_pin_attempts);
    fprintf(fp, "OTP_THRESHOLD=%.2f\n", config->otp_threshold);
    fprintf(fp, "OTP_EXPIRY_SEC=%d\n", config->otp_expiry_sec);
    fprintf(fp, "SAVINGS_MIN_BALANCE=%.2f\n", config->savings_min_balance);
    fprintf(fp, "CURRENT_MIN_BALANCE=%.2f\n", config->current_min_balance);
    fprintf(fp, "STUDENT_MIN_BALANCE=%.2f\n", config->student_min_balance);
    fprintf(fp, "SAVINGS_DAILY_LIMIT=%.2f\n", config->savings_daily_limit);
    fprintf(fp, "CURRENT_DAILY_LIMIT=%.2f\n", config->current_daily_limit);
    fprintf(fp, "STUDENT_DAILY_LIMIT=%.2f\n", config->student_daily_limit);
    fprintf(fp, "TRANSFER_FEE_PERCENT=%.2f\n", config->transfer_fee_percent);
    fprintf(fp, "FREE_WITHDRAWALS_PER_MONTH=%d\n", config->free_withdrawals_per_month);
    fprintf(fp, "EXTRA_WITHDRAWAL_FEE=%.2f\n", config->extra_withdrawal_fee);
    fprintf(fp, "SAVINGS_ANNUAL_INTEREST=%.2f\n", config->savings_annual_interest);
    fprintf(fp, "CURRENT_ANNUAL_INTEREST=%.2f\n", config->current_annual_interest);
    fprintf(fp, "STUDENT_ANNUAL_INTEREST=%.2f\n", config->student_annual_interest);
    fprintf(fp, "FRAUD_VELOCITY_COUNT=%d\n", config->fraud_velocity_count);
    fprintf(fp, "FRAUD_VELOCITY_SECONDS=%d\n", config->fraud_velocity_seconds);
    fprintf(fp, "FRAUD_TRANSFER_PCT=%.2f\n", config->fraud_transfer_pct);
    fprintf(fp, "LOW_NOTES_THRESHOLD=%d\n", config->low_notes_threshold);

    fflush(fp);
    fclose(fp);
    return true;
}

/**
 * @brief Sets default FX rates.
 */
void load_default_rates(CurrencyRates *rates) {
    if (!rates) return;
    rates->bdt_per_usd = 121.50;
    rates->bdt_per_eur = 131.20;
}

/**
 * @brief Loads rates.txt FX table.
 */
bool load_rates_file(const char *filename, CurrencyRates *rates) {
    if (!rates) return false;
    load_default_rates(rates);

    FILE *fp = fopen(filename, "r");
    if (!fp) {
        save_rates_file(filename, rates);
        return true;
    }

    char line[128];
    while (fgets(line, sizeof(line), fp)) {
        char key[32], val[64];
        if (sscanf(line, "%31[^=]=%63s", key, val) == 2) {
            if (strcmp(key, "USD_TO_BDT") == 0) {
                double r = strtod(val, NULL);
                if (r > 0.0) rates->bdt_per_usd = r;
            } else if (strcmp(key, "EUR_TO_BDT") == 0) {
                double r = strtod(val, NULL);
                if (r > 0.0) rates->bdt_per_eur = r;
            }
        }
    }
    fclose(fp);
    return true;
}

/**
 * @brief Saves updated rates to rates.txt.
 */
bool save_rates_file(const char *filename, const CurrencyRates *rates) {
    if (!rates) return false;
    FILE *fp = fopen(filename, "w");
    if (!fp) return false;
    fprintf(fp, "# Foreign Exchange Rates Table\n");
    fprintf(fp, "USD_TO_BDT=%.2f\n", rates->bdt_per_usd);
    fprintf(fp, "EUR_TO_BDT=%.2f\n", rates->bdt_per_eur);
    fflush(fp);
    fclose(fp);
    return true;
}

/* ========================================================================== */
/*                             AUDIT LOGGING (A7)                             */
/* ========================================================================== */

/**
 * @brief Appends an immutable audit log entry to audit.log with timestamp.
 */
void audit_log(const char *action, int account_num, const char *details) {
    FILE *fp = fopen(AUDIT_LOG_FILE, "a");
    if (!fp) return;

    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    char time_str[32] = "N/A";
    if (t) {
        strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", t);
    }

    fprintf(fp, "[%s] [ACTION: %-18s] [ACC: %-6d] %s\n",
            time_str,
            action ? action : "UNKNOWN",
            account_num,
            details ? details : "");

    fflush(fp);
    fclose(fp);
}

/* ========================================================================== */
/*               DATA INTEGRITY, PERSISTENCE & RECOVERY (A18)                 */
/* ========================================================================== */

/**
 * @brief Computes an Adler-32 integrity checksum over binary bytes.
 */
uint32_t calculate_checksum(const void *data, size_t bytes) {
    const uint8_t *buffer = (const uint8_t *)data;
    uint32_t a = 1, b = 0;
    for (size_t i = 0; i < bytes; i++) {
        a = (a + buffer[i]) % 65521U;
        b = (b + a) % 65521U;
    }
    return (b << 16) | a;
}

/* Forward reference to global terminal context */
static AtmContext *g_atm_ctx = NULL;

void set_atm_context_ref(AtmContext *ctx) {
    g_atm_ctx = ctx;
}

/**
 * @brief Saves all accounts and global disputes safely via accounts.tmp and atomic rename.
 */
bool save_all_data(const char *filename) {
    FILE *fp = fopen(ACCOUNTS_TEMP_FILE, "wb");
    if (!fp) {
        audit_log("SAVE_ERROR", 0, "Failed to create accounts.tmp");
        return false;
    }

    /* Compute checksum across memory payload */
    uint32_t payload_checksum = 0;
    if (g_accounts && g_account_count > 0) {
        payload_checksum ^= calculate_checksum(g_accounts, (size_t)g_account_count * sizeof(Account));
    }
    if (g_atm_ctx && g_atm_ctx->dispute_count > 0) {
        payload_checksum ^= calculate_checksum(g_atm_ctx->disputes, (size_t)g_atm_ctx->dispute_count * sizeof(Dispute));
    }

    FileHeader header;
    memset(&header, 0, sizeof(header));
    header.magic = ATM_MAGIC_HEADER;
    header.version = ATM_VERSION;
    header.account_count = (uint32_t)g_account_count;
    header.dispute_count = g_atm_ctx ? (uint32_t)g_atm_ctx->dispute_count : 0U;
    header.checksum = payload_checksum;
    header.notes_1000 = g_atm_ctx ? g_atm_ctx->cash.notes_1000 : 100;
    header.notes_500 = g_atm_ctx ? g_atm_ctx->cash.notes_500 : 200;
    header.notes_100 = g_atm_ctx ? g_atm_ctx->cash.notes_100 : 500;
    header.atm_status = g_atm_ctx ? (int)g_atm_ctx->operational_status : (int)ATM_OPERATIONAL;
    header.saved_at = time(NULL);

    /* 1. Write Header */
    if (fwrite(&header, sizeof(FileHeader), 1, fp) != 1) {
        fclose(fp);
        remove(ACCOUNTS_TEMP_FILE);
        return false;
    }

    /* 2. Write Accounts Array */
    if (g_account_count > 0 && g_accounts) {
        if (fwrite(g_accounts, sizeof(Account), (size_t)g_account_count, fp) != (size_t)g_account_count) {
            fclose(fp);
            remove(ACCOUNTS_TEMP_FILE);
            return false;
        }
    }

    /* 3. Write Global Disputes */
    if (header.dispute_count > 0 && g_atm_ctx) {
        if (fwrite(g_atm_ctx->disputes, sizeof(Dispute), (size_t)header.dispute_count, fp) != (size_t)header.dispute_count) {
            fclose(fp);
            remove(ACCOUNTS_TEMP_FILE);
            return false;
        }
    }

    if (fflush(fp) != 0) {
        fclose(fp);
        remove(ACCOUNTS_TEMP_FILE);
        return false;
    }
    fclose(fp);

    /* Verify the written temporary file before replacing active database */
    FILE *verify_fp = fopen(ACCOUNTS_TEMP_FILE, "rb");
    if (!verify_fp) return false;
    FileHeader v_hdr;
    if (fread(&v_hdr, sizeof(FileHeader), 1, verify_fp) != 1 ||
        v_hdr.magic != ATM_MAGIC_HEADER || v_hdr.version != ATM_VERSION) {
        fclose(verify_fp);
        remove(ACCOUNTS_TEMP_FILE);
        return false;
    }
    fclose(verify_fp);

    /* If primary accounts.dat exists and is valid, backup to accounts.bak */
    FILE *cur_fp = fopen(filename, "rb");
    if (cur_fp) {
        FileHeader cur_hdr;
        if (fread(&cur_hdr, sizeof(FileHeader), 1, cur_fp) == 1 && cur_hdr.magic == ATM_MAGIC_HEADER) {
            /* Verified valid primary database, backup safe to create */
            fclose(cur_fp);
            remove(ACCOUNTS_BACKUP_FILE);
            rename(filename, ACCOUNTS_BACKUP_FILE);
        } else {
            fclose(cur_fp);
        }
    }

    /* Replace active database */
    remove(filename);
    if (rename(ACCOUNTS_TEMP_FILE, filename) != 0) {
        audit_log("RENAME_FAIL", 0, "Failed to rename accounts.tmp to active database");
        return false;
    }

    return true;
}

/**
 * @brief Loads accounts and disputes from file with checksum verification.
 */
bool load_all_data(const char *filename) {
    FILE *fp = fopen(filename, "rb");
    if (!fp) return false;

    FileHeader header;
    if (fread(&header, sizeof(FileHeader), 1, fp) != 1) {
        fclose(fp);
        return false;
    }

    if (header.magic != ATM_MAGIC_HEADER || header.version != ATM_VERSION) {
        fclose(fp);
        audit_log("CORRUPT_HEADER", 0, "Magic or version mismatch in database file");
        return false;
    }

    if (header.account_count > 1000 || header.dispute_count > MAX_DISPUTES_GLOBAL) {
        fclose(fp);
        audit_log("CORRUPT_COUNT", 0, "Record counts exceed physical safety bounds");
        return false;
    }

    /* Allocate buffer for accounts */
    Account *temp_accounts = NULL;
    if (header.account_count > 0) {
        temp_accounts = (Account *)malloc((size_t)header.account_count * sizeof(Account));
        if (!temp_accounts) {
            fclose(fp);
            return false;
        }
        if (fread(temp_accounts, sizeof(Account), (size_t)header.account_count, fp) != (size_t)header.account_count) {
            free(temp_accounts);
            fclose(fp);
            return false;
        }
    }

    /* Allocate buffer for disputes */
    Dispute temp_disputes[MAX_DISPUTES_GLOBAL];
    if (header.dispute_count > 0) {
        if (fread(temp_disputes, sizeof(Dispute), (size_t)header.dispute_count, fp) != (size_t)header.dispute_count) {
            free(temp_accounts);
            fclose(fp);
            return false;
        }
    }

    fclose(fp);

    /* Verify payload checksum */
    uint32_t calc = 0;
    if (temp_accounts && header.account_count > 0) {
        calc ^= calculate_checksum(temp_accounts, (size_t)header.account_count * sizeof(Account));
    }
    if (header.dispute_count > 0) {
        calc ^= calculate_checksum(temp_disputes, (size_t)header.dispute_count * sizeof(Dispute));
    }

    if (calc != header.checksum) {
        free(temp_accounts);
        audit_log("CHECKSUM_MISMATCH", 0, "Database payload checksum failed verification");
        return false;
    }

    /* Replace live data */
    free(g_accounts);
    g_accounts = temp_accounts;
    g_account_count = (int)header.account_count;
    g_account_capacity = g_account_count;

    if (g_atm_ctx) {
        g_atm_ctx->cash.notes_1000 = header.notes_1000;
        g_atm_ctx->cash.notes_500 = header.notes_500;
        g_atm_ctx->cash.notes_100 = header.notes_100;
        g_atm_ctx->operational_status = (AtmStatus)header.atm_status;
        g_atm_ctx->dispute_count = (int)header.dispute_count;
        if (header.dispute_count > 0) {
            memcpy(g_atm_ctx->disputes, temp_disputes, (size_t)header.dispute_count * sizeof(Dispute));
        }
    }

    audit_log("DATA_LOADED", 0, "Database loaded and verified successfully");
    return true;
}

/**
 * @brief Recovers database from verified accounts.bak when primary is damaged.
 */
bool recover_from_backup(void) {
    audit_log("RECOVERY_START", 0, "Attempting recovery from accounts.bak");
    printf("\n[!] WARNING: Primary database missing or corrupted!\n");
    printf("[*] Inspecting backup file: %s...\n", ACCOUNTS_BACKUP_FILE);

    if (load_all_data(ACCOUNTS_BACKUP_FILE)) {
        printf("[+] SUCCESS: Data integrity verified from backup!\n");
        printf("[+] Restoring active database to %s...\n", ACCOUNTS_FILE);
        save_all_data(ACCOUNTS_FILE);
        audit_log("RECOVERY_SUCCESS", 0, "Restored active database from accounts.bak");
        return true;
    }

    printf("[-] FAILURE: Backup file is also missing or invalid.\n");
    audit_log("RECOVERY_FAIL", 0, "Both primary and backup database corrupted/missing");
    return false;
}

/**
 * @brief Initializes 3 default demo accounts when starting clean.
 */
void init_demo_accounts(void) {
    printf("[*] Initializing fresh database with 3 demo accounts...\n");
    create_account(1001, "Alice Rahman", "1234", ACCOUNT_SAVINGS, 25000.0);
    create_account(1002, "Bob Chowdhury", "4321", ACCOUNT_CURRENT, 85000.0);
    create_account(1003, "Charlie Hasan", "1111", ACCOUNT_STUDENT, 5000.0);

    /* Record demo transactions */
    Account *a1 = find_account(1001);
    if (a1) {
        char ref[MAX_REF_LEN];
        record_transaction(a1, TXN_DEPOSIT, 25000.0, 0.0, 0, "Initial Account Opening Deposit", ref);
    }
    Account *a2 = find_account(1002);
    if (a2) {
        char ref[MAX_REF_LEN];
        record_transaction(a2, TXN_DEPOSIT, 85000.0, 0.0, 0, "Initial Account Opening Deposit", ref);
    }
    Account *a3 = find_account(1003);
    if (a3) {
        char ref[MAX_REF_LEN];
        record_transaction(a3, TXN_DEPOSIT, 5000.0, 0.0, 0, "Initial Account Opening Deposit", ref);
    }

    save_all_data(ACCOUNTS_FILE);
    audit_log("DEMO_INIT", 0, "Initialized 3 demo accounts (1001, 1002, 1003)");
}

/**
 * @brief Releases all allocated memory cleanly before program termination.
 */
void cleanup_system_memory(void) {
    if (g_accounts) {
        free(g_accounts);
        g_accounts = NULL;
    }
    g_account_count = 0;
    g_account_capacity = 0;
}

/* ========================================================================== */
/*                    INPUT VALIDATION & CONSOLE HELPERS                      */
/* ========================================================================== */

void clear_screen(void) {
    /* Safe ANSI escape code sequence to clear screen and move cursor home */
    printf("\033[2J\033[H");
}

void press_enter_to_continue(void) {
    if (feof(stdin)) return;
    printf("\nPress [Enter] to continue...");
    int ch;
    while ((ch = getchar()) != '\n' && ch != EOF);
}

bool get_line_input(char *buffer, size_t size) {
    if (!buffer || size == 0) return false;
    if (!fgets(buffer, (int)size, stdin)) return false;

    /* Strip trailing newline or carriage return */
    buffer[strcspn(buffer, "\r\n")] = '\0';
    return true;
}

bool get_int_input(const char *prompt, int *out_val, int min_val, int max_val) {
    char buf[64];
    while (1) {
        printf("%s", prompt);
        if (!get_line_input(buf, sizeof(buf))) return false;
        if (buf[0] == '\0') {
            printf("[!] Input cannot be empty. Please try again.\n");
            continue;
        }

        char *endptr = NULL;
        long val = strtol(buf, &endptr, 10);
        if (*endptr != '\0') {
            printf("[!] Invalid numeric input. Numbers only.\n");
            continue;
        }
        if (val < min_val || val > max_val) {
            printf("[!] Value must be between %d and %d.\n", min_val, max_val);
            continue;
        }
        *out_val = (int)val;
        return true;
    }
}

bool get_double_input(const char *prompt, double *out_val, double min_val, double max_val) {
    char buf[64];
    while (1) {
        printf("%s", prompt);
        if (!get_line_input(buf, sizeof(buf))) return false;
        if (buf[0] == '\0') {
            printf("[!] Input cannot be empty. Please try again.\n");
            continue;
        }

        char *endptr = NULL;
        double val = strtod(buf, &endptr);
        if (*endptr != '\0') {
            printf("[!] Invalid decimal input. Numbers only.\n");
            continue;
        }
        if (val < min_val || val > max_val) {
            printf("[!] Value must be between %.2f and %.2f.\n", min_val, max_val);
            continue;
        }
        *out_val = val;
        return true;
    }
}

void print_header(const char *title) {
    printf("\n================================================================================\n");
    printf("                           %-30s\n", title ? title : "ATM SYSTEM");
    printf("================================================================================\n");
}

void print_receipt(const Transaction *txn, const Account *acc) {
    if (!txn || !acc) return;
    time_t ts = txn->timestamp;
    struct tm *t = localtime(&ts);
    char time_str[32] = "N/A";
    if (t) strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", t);

    printf("\n+-------------------------------------------------------------+\n");
    printf("|                       OFFICIAL ATM RECEIPT                  |\n");
    printf("+-------------------------------------------------------------+\n");
    printf("| Reference ID : %-44s |\n", txn->ref_id);
    printf("| Date & Time  : %-44s |\n", time_str);
    printf("| Account Num  : %-44d |\n", acc->account_number);
    printf("| Customer Name: %-44s |\n", acc->name);
    printf("| Type         : %-44s |\n",
           txn->type == TXN_DEPOSIT ? "Deposit" :
           txn->type == TXN_WITHDRAWAL ? "Cash Withdrawal" :
           txn->type == TXN_TRANSFER_OUT ? "Transfer Sent" :
           txn->type == TXN_TRANSFER_IN ? "Transfer Received" :
           txn->type == TXN_BILL_PAYMENT ? "Bill Payment" :
           txn->type == TXN_REVERSAL ? "Transaction Reversal" :
           txn->type == TXN_INTEREST_CREDIT ? "Interest Credit" :
           txn->type == TXN_FEE_CHARGE ? "Service Fee" : "FD Placement");
    printf("| Amount       : BDT %-40.2f |\n", txn->amount);
    printf("| Service Fee  : BDT %-40.2f |\n", txn->fee);
    printf("| New Balance  : BDT %-40.2f |\n", txn->balance_after);
    printf("| Note / Info  : %-44s |\n", txn->description);
    printf("+-------------------------------------------------------------+\n");
    printf("|           Thank you for banking with Antigravity Bank       |\n");
    printf("+-------------------------------------------------------------+\n");
}

/* ========================================================================== */
/*                       ACCOUNT QUERY & MANAGEMENT                           */
/* ========================================================================== */

Account *find_account(int account_number) {
    if (!g_accounts || g_account_count <= 0) return NULL;
    for (int i = 0; i < g_account_count; i++) {
        if (g_accounts[i].account_number == account_number) {
            return &g_accounts[i];
        }
    }
    return NULL;
}

Account *create_account(int account_number, const char *name, const char *pin, AccountType type, double initial_deposit) {
    if (find_account(account_number)) return NULL;

    if (g_account_count >= g_account_capacity) {
        int new_cap = (g_account_capacity == 0) ? 8 : g_account_capacity * 2;
        Account *new_arr = (Account *)realloc(g_accounts, (size_t)new_cap * sizeof(Account));
        if (!new_arr) return NULL;
        g_accounts = new_arr;
        g_account_capacity = new_cap;
    }

    Account *acc = &g_accounts[g_account_count++];
    memset(acc, 0, sizeof(Account));

    acc->account_number = account_number;
    strncpy(acc->name, name ? name : "Unnamed", MAX_NAME_LEN - 1);
    acc->name[MAX_NAME_LEN - 1] = '\0';

    generate_salt(acc->pin_salt, 16);
    compute_pin_hash(pin ? pin : "0000", acc->pin_salt, acc->pin_hash);

    acc->must_change_pin = false;
    acc->type = type;
    acc->status = STATUS_ACTIVE;
    acc->balance = initial_deposit;
    acc->low_balance_threshold = 1000.0;
    acc->failed_pin_attempts = 0;
    acc->failed_otp_attempts = 0;
    acc->daily_withdrawn = 0.0;
    acc->daily_transferred = 0.0;
    acc->last_daily_reset = time(NULL);
    acc->monthly_withdrawals_count = 0;
    acc->withdrawal_ts_count = 0;

    return acc;
}

bool delete_account(int account_number) {
    int idx = -1;
    for (int i = 0; i < g_account_count; i++) {
        if (g_accounts[i].account_number == account_number) {
            idx = i;
            break;
        }
    }
    if (idx == -1) return false;

    /* Shift remaining accounts down */
    for (int i = idx; i < g_account_count - 1; i++) {
        g_accounts[i] = g_accounts[i + 1];
    }
    g_account_count--;
    return true;
}

double get_min_balance_for_type(AccountType type, const BankConfig *config) {
    if (!config) return 500.0;
    switch (type) {
        case ACCOUNT_SAVINGS: return config->savings_min_balance;
        case ACCOUNT_CURRENT: return config->current_min_balance;
        case ACCOUNT_STUDENT: return config->student_min_balance;
        default: return 500.0;
    }
}

double get_daily_limit_for_type(AccountType type, const BankConfig *config) {
    if (!config) return 20000.0;
    switch (type) {
        case ACCOUNT_SAVINGS: return config->savings_daily_limit;
        case ACCOUNT_CURRENT: return config->current_daily_limit;
        case ACCOUNT_STUDENT: return config->student_daily_limit;
        default: return 20000.0;
    }
}

void check_and_reset_daily_limits(Account *acc) {
    if (!acc) return;
    time_t now = time(NULL);
    struct tm t_now, t_last;
    localtime_s(&t_now, &now);
    localtime_s(&t_last, &acc->last_daily_reset);

    if (t_now.tm_year != t_last.tm_year || t_now.tm_yday != t_last.tm_yday) {
        acc->daily_withdrawn = 0.0;
        acc->daily_transferred = 0.0;
        acc->last_daily_reset = now;
    }
}

/* ========================================================================== */
/*                         CASH INVENTORY & DISPENSER                         */
/* ========================================================================== */

/**
 * @brief Computes optimal note breakdown and checks hardware availability (A10).
 */
bool dispense_cash(double amount, CashInventory *inventory, int *out_1000, int *out_500, int *out_100) {
    if (!inventory || !out_1000 || !out_500 || !out_100) return false;
    long remaining = (long)amount;
    if (remaining % 100 != 0 || remaining <= 0) return false;

    int n1000 = 0, n500 = 0, n100 = 0;
    int avail_1000 = inventory->notes_1000;
    int avail_500 = inventory->notes_500;
    int avail_100 = inventory->notes_100;

    /* Greedy note dispensing */
    int take1000 = (int)(remaining / 1000);
    if (take1000 > avail_1000) take1000 = avail_1000;
    remaining -= (long)take1000 * 1000;
    n1000 = take1000;

    int take500 = (int)(remaining / 500);
    if (take500 > avail_500) take500 = avail_500;
    remaining -= (long)take500 * 500;
    n500 = take500;

    int take100 = (int)(remaining / 100);
    if (take100 > avail_100) take100 = avail_100;
    remaining -= (long)take100 * 100;
    n100 = take100;

    if (remaining != 0) {
        /* Cannot dispense exact denomination breakdown with current stock */
        return false;
    }

    /* Deduct from physical inventory */
    inventory->notes_1000 -= n1000;
    inventory->notes_500 -= n500;
    inventory->notes_100 -= n100;

    *out_1000 = n1000;
    *out_500 = n500;
    *out_100 = n100;
    return true;
}

void refill_cash(CashInventory *inventory, int add_1000, int add_500, int add_100) {
    if (!inventory) return;
    if (add_1000 > 0 && inventory->notes_1000 <= 10000 - add_1000) inventory->notes_1000 += add_1000;
    if (add_500 > 0 && inventory->notes_500 <= 10000 - add_500) inventory->notes_500 += add_500;
    if (add_100 > 0 && inventory->notes_100 <= 10000 - add_100) inventory->notes_100 += add_100;
}

int get_total_cash_value(const CashInventory *inventory) {
    if (!inventory) return 0;
    return (inventory->notes_1000 * 1000) + (inventory->notes_500 * 500) + (inventory->notes_100 * 100);
}

/* ========================================================================== */
/*                             TRANSACTIONS ENGINE                            */
/* ========================================================================== */

bool record_transaction(Account *acc, TransactionType type, double amount, double fee,
                        int related_acc, const char *desc, char *out_ref) {
    if (!acc) return false;

    if (acc->txn_count >= MAX_TXNS_PER_ACC) {
        /* Shift older transactions down to maintain FIFO history */
        for (int i = 0; i < MAX_TXNS_PER_ACC - 1; i++) {
            acc->transactions[i] = acc->transactions[i + 1];
        }
        acc->txn_count = MAX_TXNS_PER_ACC - 1;
    }

    Transaction *t = &acc->transactions[acc->txn_count++];
    memset(t, 0, sizeof(Transaction));

    generate_reference_id(t->ref_id, sizeof(t->ref_id));
    t->type = type;
    t->amount = amount;
    t->fee = fee;
    t->balance_after = acc->balance;
    t->timestamp = time(NULL);
    t->related_account = related_acc;
    t->is_reversed = false;
    t->reversal_ref[0] = '\0';

    if (desc) {
        strncpy(t->description, desc, MAX_DESC_LEN - 1);
        t->description[MAX_DESC_LEN - 1] = '\0';
    }

    if (out_ref) {
        strncpy(out_ref, t->ref_id, MAX_REF_LEN - 1);
        out_ref[MAX_REF_LEN - 1] = '\0';
    }

    return true;
}

bool perform_deposit(Account *acc, double amount) {
    if (!acc || amount <= 0.0) return false;
    acc->balance += amount;

    char ref[MAX_REF_LEN];
    record_transaction(acc, TXN_DEPOSIT, amount, 0.0, 0, "Cash Deposit", ref);

    char log_buf[128];
    snprintf(log_buf, sizeof(log_buf), "Deposit: BDT %.2f | Ref: %s", amount, ref);
    audit_log("DEPOSIT", acc->account_number, log_buf);

    add_notification(acc, "Deposit successful. Balance credited.");
    save_all_data(ACCOUNTS_FILE);

    Transaction *t = &acc->transactions[acc->txn_count - 1];
    print_receipt(t, acc);
    return true;
}

bool perform_withdrawal(Account *acc, double amount, AtmContext *atm) {
    if (!acc || !atm || amount <= 0.0) return false;

    /* A22: Check ATM operational mode */
    if (atm->operational_status == ATM_MAINTENANCE) {
        printf("\n[!] NOTICE: This ATM is currently in MAINTENANCE mode.\n");
        printf("[!] Cash withdrawals are temporarily suspended. Inquiries only.\n");
        audit_log("WITHDRAW_FAIL", acc->account_number, "Rejected: ATM in Maintenance mode");
        return false;
    }

    /* Multiples of 100 check */
    long amt_int = (long)amount;
    if (amt_int % 100 != 0) {
        printf("\n[!] Error: Withdrawals must be in multiples of 100 BDT.\n");
        return false;
    }

    check_and_reset_daily_limits(acc);

    /* Daily withdrawal limit check */
    double daily_lim = get_daily_limit_for_type(acc->type, &atm->config);
    if (acc->daily_withdrawn + amount > daily_lim) {
        printf("\n[!] Error: Withdrawal exceeds your daily limit of BDT %.2f.\n", daily_lim);
        printf("[!] Withdrawn today: BDT %.2f | Requested: BDT %.2f\n", acc->daily_withdrawn, amount);
        audit_log("WITHDRAW_FAIL", acc->account_number, "Exceeded daily limit");
        return false;
    }

    /* A4: Fee engine for withdrawals */
    double fee = 0.0;
    if (acc->monthly_withdrawals_count >= atm->config.free_withdrawals_per_month) {
        fee = atm->config.extra_withdrawal_fee;
        printf("[i] Monthly free withdrawal limit exceeded. Fee applied: BDT %.2f\n", fee);
    }

    /* Minimum balance check */
    double min_bal = get_min_balance_for_type(acc->type, &atm->config);
    if (acc->balance - (amount + fee) < min_bal) {
        printf("\n[!] Error: Insufficient funds. Minimum required balance is BDT %.2f.\n", min_bal);
        printf("[!] Current Balance: BDT %.2f | Required with fee: BDT %.2f\n", acc->balance, amount + fee);
        audit_log("WITHDRAW_FAIL", acc->account_number, "Insufficient balance or min balance violation");
        return false;
    }

    /* A2: Check fraud velocity (3 withdrawals within 120s) */
    time_t now = time(NULL);
    if (acc->withdrawal_ts_count >= 2) {
        time_t first_ts = acc->withdrawal_timestamps[0];
        if (now - first_ts <= atm->config.fraud_velocity_seconds) {
            acc->status = STATUS_FROZEN_FRAUD;
            printf("\n[!] FRAUD ALERT: High withdrawal velocity detected (3 attempts within %d seconds)!\n",
                   atm->config.fraud_velocity_seconds);
            printf("[!] Your account has been automatically FROZEN for security. Contact admin.\n");
            audit_log("FRAUD_FREEZE", acc->account_number, "Auto-frozen: 3 withdrawals within velocity window");
            add_notification(acc, "ACCOUNT FROZEN: Fraudulent withdrawal velocity detected.");
            save_all_data(ACCOUNTS_FILE);
            return false;
        }
    }

    /* A10: Note dispenser check */
    int n1000 = 0, n500 = 0, n100 = 0;
    if (!dispense_cash(amount, &atm->cash, &n1000, &n500, &n100)) {
        printf("\n[!] ATM Error: Unable to dispense exact note combination with current cash stock.\n");
        printf("[!] Available ATM cash: BDT %d. Please try a different amount.\n", get_total_cash_value(&atm->cash));
        audit_log("CASH_DISP_FAIL", acc->account_number, "Cassette notes cannot form requested amount");
        return false;
    }

    /* Commit withdrawal */
    acc->balance -= (amount + fee);
    acc->daily_withdrawn += amount;
    acc->monthly_withdrawals_count++;

    /* Record velocity timestamp */
    if (acc->withdrawal_ts_count < 3) {
        acc->withdrawal_timestamps[acc->withdrawal_ts_count++] = now;
    } else {
        acc->withdrawal_timestamps[0] = acc->withdrawal_timestamps[1];
        acc->withdrawal_timestamps[1] = acc->withdrawal_timestamps[2];
        acc->withdrawal_timestamps[2] = now;
    }

    char ref[MAX_REF_LEN];
    char desc[MAX_DESC_LEN];
    snprintf(desc, sizeof(desc), "ATM Withdrawal (%dx1k, %dx500, %dx100)", n1000, n500, n100);
    record_transaction(acc, TXN_WITHDRAWAL, amount, fee, 0, desc, ref);

    char log_buf[128];
    snprintf(log_buf, sizeof(log_buf), "Withdraw: BDT %.2f (Fee: %.2f) | Ref: %s", amount, fee, ref);
    audit_log("WITHDRAWAL", acc->account_number, log_buf);

    /* A15: Low-balance alert */
    if (acc->balance < acc->low_balance_threshold) {
        printf("\n[!] ALERT: Your account balance (BDT %.2f) is below your alert threshold (BDT %.2f)!\n",
               acc->balance, acc->low_balance_threshold);
        add_notification(acc, "Low-balance threshold crossed.");
    }

    /* A10, A22: Check low stock warning */
    if (atm->cash.notes_1000 < atm->config.low_notes_threshold ||
        atm->cash.notes_500 < atm->config.low_notes_threshold ||
        atm->cash.notes_100 < atm->config.low_notes_threshold) {
        printf("\n[!] Hardware Warning: ATM cash cassettes are running low.\n");
    }

    save_all_data(ACCOUNTS_FILE);
    Transaction *t = &acc->transactions[acc->txn_count - 1];
    print_receipt(t, acc);
    return true;
}

bool perform_transfer(Account *sender, Account *recipient, double amount, AtmContext *atm) {
    if (!sender || !recipient || !atm || amount <= 0.0) return false;

    if (sender->account_number == recipient->account_number) {
        printf("\n[!] Error: Cannot transfer money to the same account.\n");
        return false;
    }

    if (recipient->status != STATUS_ACTIVE) {
        printf("\n[!] Error: Destination account is not active or blocked.\n");
        return false;
    }

    check_and_reset_daily_limits(sender);

    /* A4: Transfer fee */
    double fee = (amount * atm->config.transfer_fee_percent) / 100.0;
    double total_deduct = amount + fee;

    /* Minimum balance check */
    double min_bal = get_min_balance_for_type(sender->type, &atm->config);
    if (sender->balance - total_deduct < min_bal) {
        printf("\n[!] Error: Insufficient funds. Transfer amount + fee (BDT %.2f) breaches minimum balance (BDT %.2f).\n",
               total_deduct, min_bal);
        audit_log("TRANSFER_FAIL", sender->account_number, "Insufficient balance");
        return false;
    }

    /* A2: Fraud rule - transfer > 80% of balance */
    if (amount > (sender->balance * atm->config.fraud_transfer_pct)) {
        sender->status = STATUS_FROZEN_FRAUD;
        printf("\n[!] FRAUD ALERT: High-risk transfer anomaly detected (> %.0f%% of total balance)!\n",
               atm->config.fraud_transfer_pct * 100.0);
        printf("[!] Your account has been automatically FROZEN for security. Please visit branch.\n");
        audit_log("FRAUD_FREEZE", sender->account_number, "Auto-frozen: Transfer exceeded 80% balance threshold");
        add_notification(sender, "ACCOUNT FROZEN: Suspicious high-value transfer detected.");
        save_all_data(ACCOUNTS_FILE);
        return false;
    }

    /* A1: OTP verification for transfers above threshold */
    if (amount >= atm->config.otp_threshold) {
        if (!verify_otp_flow(sender, amount, &atm->config)) {
            /* OTP failed or cancelled */
            return false;
        }
    }

    /* Commit transfer */
    sender->balance -= total_deduct;
    sender->daily_transferred += amount;
    recipient->balance += amount;

    char ref_out[MAX_REF_LEN];
    char desc_out[MAX_DESC_LEN];
    snprintf(desc_out, sizeof(desc_out), "Transfer to Acc #%d (%.40s)", recipient->account_number, recipient->name);
    record_transaction(sender, TXN_TRANSFER_OUT, amount, fee, recipient->account_number, desc_out, ref_out);

    char ref_in[MAX_REF_LEN];
    char desc_in[MAX_DESC_LEN];
    snprintf(desc_in, sizeof(desc_in), "Transfer from Acc #%d (%.40s)", sender->account_number, sender->name);
    record_transaction(recipient, TXN_TRANSFER_IN, amount, 0.0, sender->account_number, desc_in, ref_in);

    char log_buf[128];
    snprintf(log_buf, sizeof(log_buf), "Transfer Sent: BDT %.2f to Acc #%d | Ref: %s", amount, recipient->account_number, ref_out);
    audit_log("TRANSFER_OUT", sender->account_number, log_buf);
    snprintf(log_buf, sizeof(log_buf), "Transfer Recv: BDT %.2f from Acc #%d | Ref: %s", amount, sender->account_number, ref_in);
    audit_log("TRANSFER_IN", recipient->account_number, log_buf);

    add_notification(recipient, "Funds received via electronic transfer.");
    save_all_data(ACCOUNTS_FILE);

    Transaction *t = &sender->transactions[sender->txn_count - 1];
    print_receipt(t, sender);
    return true;
}

bool perform_bill_payment(Account *acc, BillCategory cat, const char *biller, const char *ref_num, double amount) {
    if (!acc || amount <= 0.0) return false;

    if (acc->balance - amount < 100.0) {
        printf("\n[!] Error: Insufficient funds for bill payment.\n");
        return false;
    }

    acc->balance -= amount;

    const char *cat_str = (cat == BILL_ELECTRICITY ? "Electricity" :
                           cat == BILL_GAS ? "Gas" :
                           cat == BILL_INTERNET ? "Internet" : "Mobile Recharge");

    char desc[MAX_DESC_LEN];
    snprintf(desc, sizeof(desc), "Bill Pay: %s (%s - %s)", cat_str, biller ? biller : "Utility", ref_num ? ref_num : "Meter");

    char ref[MAX_REF_LEN];
    record_transaction(acc, TXN_BILL_PAYMENT, amount, 0.0, 0, desc, ref);

    char log_buf[256];
    snprintf(log_buf, sizeof(log_buf), "Bill Paid: BDT %.2f (%.120s) | Ref: %.32s", amount, desc, ref);
    audit_log("BILL_PAY", acc->account_number, log_buf);

    save_all_data(ACCOUNTS_FILE);
    Transaction *t = &acc->transactions[acc->txn_count - 1];
    print_receipt(t, acc);
    return true;
}

bool perform_reversal(const char *target_ref_id, const char *reason, int admin_id, AtmContext *atm) {
    if (!target_ref_id || !atm) return false;

    /* Locate transaction in active accounts */
    Account *target_acc = NULL;
    Transaction *target_txn = NULL;

    for (int i = 0; i < g_account_count; i++) {
        for (int j = 0; j < g_accounts[i].txn_count; j++) {
            if (strcmp(g_accounts[i].transactions[j].ref_id, target_ref_id) == 0) {
                target_acc = &g_accounts[i];
                target_txn = &g_accounts[i].transactions[j];
                break;
            }
        }
        if (target_txn) break;
    }

    if (!target_acc || !target_txn) {
        printf("\n[!] Error: Transaction reference '%s' not found.\n", target_ref_id);
        return false;
    }

    if (target_txn->is_reversed) {
        printf("\n[!] Error: Transaction '%s' has ALREADY been reversed (Reversal Ref: %s).\n",
               target_ref_id, target_txn->reversal_ref);
        return false;
    }

    double rev_amount = target_txn->amount + target_txn->fee;

    /* Solvency and integrity validation */
    if (target_txn->type == TXN_DEPOSIT || target_txn->type == TXN_TRANSFER_IN) {
        if (target_acc->balance < target_txn->amount) {
            printf("\n[!] Reversal Rejected: Target account #%d has insufficient balance (BDT %.2f) to reverse credit (BDT %.2f).\n",
                   target_acc->account_number, target_acc->balance, target_txn->amount);
            return false;
        }
        target_acc->balance -= target_txn->amount;
    } else if (target_txn->type == TXN_WITHDRAWAL || target_txn->type == TXN_BILL_PAYMENT || target_txn->type == TXN_TRANSFER_OUT) {
        target_acc->balance += rev_amount;
    } else {
        printf("\n[!] Reversal Rejected: Unsupported transaction type for reversal.\n");
        return false;
    }

    char rev_ref[MAX_REF_LEN];
    char desc[MAX_DESC_LEN];
    snprintf(desc, sizeof(desc), "Reversal of %s (Reason: %s)", target_ref_id, reason ? reason : "Administrative correction");

    record_transaction(target_acc, TXN_REVERSAL, rev_amount, 0.0, target_acc->account_number, desc, rev_ref);

    target_txn->is_reversed = true;
    strncpy(target_txn->reversal_ref, rev_ref, MAX_REF_LEN - 1);

    char log_buf[160];
    snprintf(log_buf, sizeof(log_buf), "Reversal: Orig Ref: %s -> Rev Ref: %s by Admin #%d | Reason: %s",
             target_ref_id, rev_ref, admin_id, reason ? reason : "None");
    audit_log("TXN_REVERSAL", target_acc->account_number, log_buf);

    add_notification(target_acc, "Transaction reversed by administration.");
    save_all_data(ACCOUNTS_FILE);

    printf("\n[+] SUCCESS: Transaction %s successfully reversed!\n", target_ref_id);
    printf("[+] Compensating Reversal Ref: %s\n", rev_ref);
    printf("[+] Customer Account #%d new balance: BDT %.2f\n", target_acc->account_number, target_acc->balance);
    return true;
}

/* ========================================================================== */
/*                     ADVANCED FEATURES IMPLEMENTATION                       */
/* ========================================================================== */

/* --- A1: OTP Verification --- */
bool verify_otp_flow(Account *acc, double amount, const BankConfig *config) {
    if (!acc || !config) return false;

    int otp = 100000 + (rand() % 900000);
    time_t generated_at = time(NULL);

    printf("\n================================================================================\n");
    printf("                           ONE-TIME PASSCODE (OTP)\n");
    printf("================================================================================\n");
    printf("[i] Transfer sum of BDT %.2f exceeds security threshold (BDT %.2f).\n", amount, config->otp_threshold);
    printf("[SMS SIMULATION] To: +88017******** | Your 6-digit OTP is: >> %06d << (Valid for %ds)\n",
           otp, config->otp_expiry_sec);
    printf("================================================================================\n");

    int tries = 3;
    while (tries > 0) {
        int entered_otp = 0;
        if (!get_int_input("Enter 6-digit OTP code (or 0 to cancel): ", &entered_otp, 0, 999999)) {
            return false;
        }

        if (entered_otp == 0) {
            printf("[*] Transaction cancelled by user.\n");
            return false;
        }

        time_t now = time(NULL);
        if (now - generated_at > config->otp_expiry_sec) {
            printf("\n[!] Error: OTP has EXPIRED (%d seconds exceeded). Transaction aborted.\n", config->otp_expiry_sec);
            audit_log("OTP_EXPIRED", acc->account_number, "OTP time window elapsed");
            return false;
        }

        if (entered_otp == otp) {
            printf("[+] OTP Verified successfully!\n");
            acc->failed_otp_attempts = 0;
            audit_log("OTP_SUCCESS", acc->account_number, "OTP passed for high-value transfer");
            return true;
        }

        tries--;
        acc->failed_otp_attempts++;
        printf("[-] Invalid OTP. %d attempt(s) remaining.\n", tries);

        /* A2: 2 failed OTPs in a row triggers fraud freeze */
        if (acc->failed_otp_attempts >= 2) {
            acc->status = STATUS_FROZEN_FRAUD;
            printf("\n[!] FRAUD SECURITY ALERT: Multiple consecutive failed OTP verifications!\n");
            printf("[!] Your account has been automatically FROZEN. Please contact bank administration.\n");
            audit_log("FRAUD_FREEZE", acc->account_number, "Auto-frozen: 2 consecutive bad OTP entries");
            add_notification(acc, "ACCOUNT FROZEN: Multiple failed OTP attempts.");
            save_all_data(ACCOUNTS_FILE);
            return false;
        }
    }

    return false;
}

/* --- A4: End of Month Interest & Fees --- */
void process_end_of_month(AtmContext *atm) {
    if (!atm || !g_accounts || g_account_count <= 0) {
        printf("\n[!] No accounts available for End of Month processing.\n");
        return;
    }

    print_header("END OF MONTH PROCESSING");
    double total_interest_credited = 0.0;
    int accounts_credited = 0;

    for (int i = 0; i < g_account_count; i++) {
        Account *a = &g_accounts[i];
        if (a->status != STATUS_ACTIVE) continue;

        double rate = 0.0;
        if (a->type == ACCOUNT_SAVINGS) rate = atm->config.savings_annual_interest;
        else if (a->type == ACCOUNT_STUDENT) rate = atm->config.student_annual_interest;

        if (rate > 0.0 && a->balance > 0.0) {
            /* Monthly interest = Balance * Annual Rate / 1200 */
            double interest = (a->balance * rate) / 1200.0;
            if (interest >= 0.01) {
                a->balance += interest;
                char ref[MAX_REF_LEN];
                record_transaction(a, TXN_INTEREST_CREDIT, interest, 0.0, 0, "Monthly Interest Credit", ref);
                total_interest_credited += interest;
                accounts_credited++;
            }
        }

        /* Reset monthly free withdrawal counter */
        a->monthly_withdrawals_count = 0;
    }

    save_all_data(ACCOUNTS_FILE);
    audit_log("END_OF_MONTH", 0, "Executed monthly interest accrual and usage resets");

    printf("\n[+] End-of-Month cycle completed successfully!\n");
    printf("[+] Accounts Credited     : %d\n", accounts_credited);
    printf("[+] Total Interest Payout : BDT %.2f\n", total_interest_credited);
    printf("[+] Monthly withdrawal counts reset for all active accounts.\n");
}

/* --- A5: Fixed Deposit & Loan EMI Calculator --- */
bool open_fixed_deposit(Account *acc, double principal, int months, double annual_rate) {
    if (!acc || acc->fd_count >= MAX_FDS_PER_ACC || principal <= 0.0 || months <= 0) return false;

    if (acc->balance - principal < 500.0) {
        printf("\n[!] Error: Insufficient cleared funds to place Fixed Deposit of BDT %.2f.\n", principal);
        return false;
    }

    acc->balance -= principal;

    FixedDeposit *fd = &acc->fixed_deposits[acc->fd_count++];
    fd->fd_id = 5000 + acc->fd_count;
    fd->principal = principal;
    fd->interest_rate = annual_rate;
    fd->duration_months = months;
    fd->start_date = time(NULL);
    fd->maturity_date = fd->start_date + (time_t)(months * 30 * 86400LL);
    fd->maturity_amount = principal + (principal * annual_rate * months) / 1200.0;
    fd->is_active = true;

    char ref[MAX_REF_LEN];
    record_transaction(acc, TXN_FD_CREATE, principal, 0.0, fd->fd_id, "Open Fixed Deposit", ref);

    char log_buf[128];
    snprintf(log_buf, sizeof(log_buf), "FD Opened: BDT %.2f for %d mos @ %.1f%% | FD#%d", principal, months, annual_rate, fd->fd_id);
    audit_log("FD_OPEN", acc->account_number, log_buf);

    save_all_data(ACCOUNTS_FILE);
    printf("\n[+] SUCCESS: Fixed Deposit #%d created successfully!\n", fd->fd_id);
    printf("[+] Principal Locked    : BDT %.2f\n", fd->principal);
    printf("[+] Term Duration       : %d Months (@ %.2f%% p.a.)\n", fd->duration_months, fd->interest_rate);
    printf("[+] Guaranteed Maturity : BDT %.2f\n", fd->maturity_amount);
    return true;
}

void calculate_loan_emi_schedule(double principal, double annual_rate, int months) {
    print_header("LOAN EMI AMORTIZATION SCHEDULE");
    if (principal <= 0.0 || months <= 0) {
        printf("[!] Invalid loan parameters.\n");
        return;
    }

    double monthly_rate = (annual_rate / 12.0) / 100.0;
    double emi = 0.0;

    if (monthly_rate > 0.0) {
        /* EMI = P * r * (1+r)^n / ((1+r)^n - 1) */
        double factor = 1.0;
        for (int i = 0; i < months; i++) factor *= (1.0 + monthly_rate);
        emi = principal * monthly_rate * factor / (factor - 1.0);
    } else {
        emi = principal / months;
    }

    double total_payment = emi * months;
    double total_interest = total_payment - principal;

    printf("Loan Principal   : BDT %12.2f | Term Length : %d Months\n", principal, months);
    printf("Interest Rate    : %14.2f%% | Monthly EMI : BDT %12.2f\n", annual_rate, emi);
    printf("Total Interest   : BDT %12.2f | Total Cost  : BDT %12.2f\n", total_interest, total_payment);
    printf("--------------------------------------------------------------------------------\n");
    printf("%-5s | %-14s | %-12s | %-14s | %-14s | %-14s\n",
           "Month", "Beginning Bal", "EMI Amount", "Principal Paid", "Interest Paid", "Ending Balance");
    printf("--------------------------------------------------------------------------------\n");

    double balance = principal;
    for (int m = 1; m <= months; m++) {
        double interest_paid = balance * monthly_rate;
        double principal_paid = emi - interest_paid;
        if (principal_paid > balance || m == months) {
            principal_paid = balance;
            emi = principal_paid + interest_paid;
        }
        double end_bal = balance - principal_paid;
        if (end_bal < 0.001) end_bal = 0.0;

        printf("%-5d | %14.2f | %12.2f | %14.2f | %14.2f | %14.2f\n",
               m, balance, emi, principal_paid, interest_paid, end_bal);
        balance = end_bal;
        if (balance <= 0.0) break;
    }
    printf("--------------------------------------------------------------------------------\n");
}

/* --- A6: Multi-Currency Balances --- */
void display_multi_currency_balances(const Account *acc, const CurrencyRates *rates) {
    if (!acc || !rates) return;
    print_header("MULTI-CURRENCY BALANCE INQUIRY");
    double bal_bdt = acc->balance;
    double bal_usd = (rates->bdt_per_usd > 0.0) ? (bal_bdt / rates->bdt_per_usd) : 0.0;
    double bal_eur = (rates->bdt_per_eur > 0.0) ? (bal_bdt / rates->bdt_per_eur) : 0.0;

    printf("Account Number      : %d\n", acc->account_number);
    printf("Account Holder      : %s\n", acc->name);
    printf("--------------------------------------------------------------------------------\n");
    printf("Base Balance (BDT)  : BDT %15.2f\n", bal_bdt);
    printf("Equivalent in USD   : USD %15.2f (Exchange Rate: 1 USD = %.2f BDT)\n", bal_usd, rates->bdt_per_usd);
    printf("Equivalent in EUR   : EUR %15.2f (Exchange Rate: 1 EUR = %.2f BDT)\n", bal_eur, rates->bdt_per_eur);
    printf("--------------------------------------------------------------------------------\n");
}

/* --- A8: Session Timeout --- */
bool check_session_timeout(AtmContext *atm) {
    if (!atm) return false;
    time_t now = time(NULL);
    if (now - atm->last_activity_time > atm->config.session_timeout_sec) {
        printf("\n================================================================================\n");
        printf("[!] SESSION TIMED OUT: %d seconds of inactivity exceeded.\n", atm->config.session_timeout_sec);
        printf("[!] You have been automatically logged out for your security.\n");
        printf("================================================================================\n");
        audit_log("TIMEOUT", 0, "Session expired due to user inactivity");
        return true;
    }
    atm->last_activity_time = now;
    return false;
}

void touch_session(AtmContext *atm) {
    if (atm) atm->last_activity_time = time(NULL);
}

/* --- A9: Statement Search, Sort & CSV Export --- */
static int compare_newest(const void *a, const void *b) {
    const Transaction *t1 = (const Transaction *)a;
    const Transaction *t2 = (const Transaction *)b;
    if (t1->timestamp < t2->timestamp) return 1;
    if (t1->timestamp > t2->timestamp) return -1;
    return 0;
}

static int compare_oldest(const void *a, const void *b) {
    const Transaction *t1 = (const Transaction *)a;
    const Transaction *t2 = (const Transaction *)b;
    if (t1->timestamp < t2->timestamp) return -1;
    if (t1->timestamp > t2->timestamp) return 1;
    return 0;
}

static int compare_amount(const void *a, const void *b) {
    const Transaction *t1 = (const Transaction *)a;
    const Transaction *t2 = (const Transaction *)b;
    if (t1->amount < t2->amount) return 1;
    if (t1->amount > t2->amount) return -1;
    return 0;
}

void search_and_export_statements(Account *acc) {
    if (!acc) return;
    print_header("STATEMENT ANALYSIS & SEARCH TOOLS");
    if (acc->txn_count == 0) {
        printf("[i] No transaction records found on this account.\n");
        return;
    }

    printf("Filter Criteria:\n");
    printf("1. View All Transactions\n");
    printf("2. Filter by Transaction Type\n");
    printf("3. Filter by Minimum Amount\n");
    int choice = 1;
    get_int_input("Choose filter option [1-3]: ", &choice, 1, 3);

    TransactionType filter_type = TXN_DEPOSIT;
    double min_amt = 0.0;
    if (choice == 2) {
        printf("Types: 0=Deposit, 1=Withdrawal, 2=Transfer Out, 3=Transfer In, 4=Bill Pay, 5=Reversal\n");
        int t = 0;
        get_int_input("Select type: ", &t, 0, 5);
        filter_type = (TransactionType)t;
    } else if (choice == 3) {
        get_double_input("Enter minimum transaction amount: ", &min_amt, 0.0, 10000000.0);
    }

    /* Filter transactions into temporary buffer */
    Transaction matches[MAX_TXNS_PER_ACC];
    int match_count = 0;
    for (int i = 0; i < acc->txn_count; i++) {
        Transaction *t = &acc->transactions[i];
        if (choice == 2 && t->type != filter_type) continue;
        if (choice == 3 && t->amount < min_amt) continue;
        matches[match_count++] = *t;
    }

    if (match_count == 0) {
        printf("\n[i] No transactions matched the selected filter criteria.\n");
        return;
    }

    printf("\nSort Order (using qsort):\n");
    printf("1. Newest First\n");
    printf("2. Oldest First\n");
    printf("3. Highest Amount First\n");
    int sort_opt = 1;
    get_int_input("Select sort option [1-3]: ", &sort_opt, 1, 3);

    if (sort_opt == 1) qsort(matches, (size_t)match_count, sizeof(Transaction), compare_newest);
    else if (sort_opt == 2) qsort(matches, (size_t)match_count, sizeof(Transaction), compare_oldest);
    else if (sort_opt == 3) qsort(matches, (size_t)match_count, sizeof(Transaction), compare_amount);

    printf("\n--------------------------------------------------------------------------------------------------------\n");
    printf("%-26s | %-19s | %-14s | %10s | %10s | %s\n",
           "Reference ID", "Date & Time", "Type", "Amount", "Balance", "Notes");
    printf("--------------------------------------------------------------------------------------------------------\n");

    for (int i = 0; i < match_count; i++) {
        Transaction *t = &matches[i];
        struct tm *tm_p = localtime(&t->timestamp);
        char dt_str[32] = "N/A";
        if (tm_p) strftime(dt_str, sizeof(dt_str), "%Y-%m-%d %H:%M:%S", tm_p);

        const char *t_name = (t->type == TXN_DEPOSIT ? "Deposit" :
                              t->type == TXN_WITHDRAWAL ? "Withdrawal" :
                              t->type == TXN_TRANSFER_OUT ? "Transfer Out" :
                              t->type == TXN_TRANSFER_IN ? "Transfer In" :
                              t->type == TXN_BILL_PAYMENT ? "Bill Pay" :
                              t->type == TXN_REVERSAL ? "Reversal" : "Interest");

        printf("%-26s | %-19s | %-14s | %10.2f | %10.2f | %s%s\n",
               t->ref_id, dt_str, t_name, t->amount, t->balance_after, t->description,
               t->is_reversed ? " [REVERSED]" : "");
    }
    printf("--------------------------------------------------------------------------------------------------------\n");

    printf("\nExport results to %s? (1=Yes, 2=No): ", STATEMENT_CSV_FILE);
    int exp = 2;
    get_int_input("", &exp, 1, 2);
    if (exp == 1) {
        FILE *csv = fopen(STATEMENT_CSV_FILE, "w");
        if (csv) {
            fprintf(csv, "ReferenceID,DateTime,Type,Amount,Fee,BalanceAfter,Reversed,Description\n");
            for (int i = 0; i < match_count; i++) {
                Transaction *t = &matches[i];
                struct tm *tm_p = localtime(&t->timestamp);
                char dt_str[32] = "N/A";
                if (tm_p) strftime(dt_str, sizeof(dt_str), "%Y-%m-%d %H:%M:%S", tm_p);
                fprintf(csv, "%s,%s,%d,%.2f,%.2f,%.2f,%s,\"%s\"\n",
                        t->ref_id, dt_str, t->type, t->amount, t->fee, t->balance_after,
                        t->is_reversed ? "TRUE" : "FALSE", t->description);
            }
            fflush(csv);
            fclose(csv);
            printf("[+] Exported %d transactions to %s successfully!\n", match_count, STATEMENT_CSV_FILE);
        } else {
            printf("[-] Failed to open %s for writing.\n", STATEMENT_CSV_FILE);
        }
    }
}

/* --- A11: Beneficiary Management --- */
void manage_beneficiaries(Account *acc) {
    if (!acc) return;
    print_header("BENEFICIARY MANAGEMENT");
    printf("1. View Saved Beneficiaries\n");
    printf("2. Add New Beneficiary\n");
    printf("3. Remove Beneficiary\n");
    printf("4. Return to Main Menu\n");

    int choice = 4;
    get_int_input("Choose option [1-4]: ", &choice, 1, 4);

    if (choice == 1) {
        printf("\nSaved Beneficiaries (%d/%d):\n", acc->beneficiary_count, MAX_BENEFICIARIES);
        if (acc->beneficiary_count == 0) {
            printf("[i] No saved beneficiaries.\n");
        } else {
            for (int i = 0; i < acc->beneficiary_count; i++) {
                printf("%d. Account #%-6d | Legal Name: %-20s | Nickname: %s\n",
                       i + 1, acc->beneficiaries[i].account_number,
                       acc->beneficiaries[i].name, acc->beneficiaries[i].nickname);
            }
        }
    } else if (choice == 2) {
        if (acc->beneficiary_count >= MAX_BENEFICIARIES) {
            printf("[!] Beneficiary list is full (%d max).\n", MAX_BENEFICIARIES);
            return;
        }

        int target_num = 0;
        get_int_input("Enter Destination Account Number: ", &target_num, 1000, 999999);
        if (target_num == acc->account_number) {
            printf("[!] You cannot add your own account as a beneficiary.\n");
            return;
        }

        /* Check duplicate */
        for (int i = 0; i < acc->beneficiary_count; i++) {
            if (acc->beneficiaries[i].account_number == target_num) {
                printf("[!] Beneficiary #%d is already saved.\n", target_num);
                return;
            }
        }

        Account *target = find_account(target_num);
        if (!target || target->status != STATUS_ACTIVE) {
            printf("[!] Account #%d not found or not active.\n", target_num);
            return;
        }

        printf("\nRecipient Verified: >> %s <<\n", target->name);
        printf("Confirm save beneficiary? (1=Yes, 2=No): ");
        int conf = 2;
        get_int_input("", &conf, 1, 2);
        if (conf == 1) {
            char nick[32];
            printf("Enter friendly nickname: ");
            get_line_input(nick, sizeof(nick));

            Beneficiary *b = &acc->beneficiaries[acc->beneficiary_count++];
            b->account_number = target->account_number;
            strncpy(b->name, target->name, MAX_NAME_LEN - 1);
            strncpy(b->nickname, nick[0] ? nick : target->name, 31);

            save_all_data(ACCOUNTS_FILE);
            printf("[+] Beneficiary saved successfully!\n");
        }
    } else if (choice == 3) {
        if (acc->beneficiary_count == 0) {
            printf("[i] No beneficiaries to remove.\n");
            return;
        }
        int del_idx = 0;
        get_int_input("Enter beneficiary number to remove [1-N]: ", &del_idx, 1, acc->beneficiary_count);
        for (int i = del_idx - 1; i < acc->beneficiary_count - 1; i++) {
            acc->beneficiaries[i] = acc->beneficiaries[i + 1];
        }
        acc->beneficiary_count--;
        save_all_data(ACCOUNTS_FILE);
        printf("[+] Beneficiary removed successfully.\n");
    }
}

/* --- A14, A15: Notifications & Account Health --- */
void add_notification(Account *acc, const char *message) {
    if (!acc || !message) return;
    if (acc->notification_count >= MAX_NOTIFICATIONS) {
        for (int i = 0; i < MAX_NOTIFICATIONS - 1; i++) {
            acc->notifications[i] = acc->notifications[i + 1];
        }
        acc->notification_count = MAX_NOTIFICATIONS - 1;
    }
    Notification *n = &acc->notifications[acc->notification_count++];
    n->notification_id = 100 + acc->notification_count;
    n->timestamp = time(NULL);
    strncpy(n->message, message, MAX_NOTE_LEN - 1);
    n->message[MAX_NOTE_LEN - 1] = '\0';
    n->is_read = false;
}

void view_notifications(Account *acc) {
    if (!acc) return;
    print_header("CUSTOMER NOTIFICATIONS & ALERTS");
    if (acc->notification_count == 0) {
        printf("[i] No notifications at this time.\n");
        return;
    }
    for (int i = 0; i < acc->notification_count; i++) {
        Notification *n = &acc->notifications[i];
        struct tm *tm_p = localtime(&n->timestamp);
        char dt[32] = "N/A";
        if (tm_p) strftime(dt, sizeof(dt), "%Y-%m-%d %H:%M", tm_p);
        printf("[%s] %s%s\n", dt, n->message, n->is_read ? "" : " *NEW*");
        n->is_read = true;
    }
    save_all_data(ACCOUNTS_FILE);
}

void display_account_health(const Account *acc, const BankConfig *config) {
    if (!acc || !config) return;
    print_header("ACCOUNT HEALTH & LIQUIDITY SUMMARY");
    double min_req = get_min_balance_for_type(acc->type, config);
    double avail = acc->balance - min_req;
    if (avail < 0.0) avail = 0.0;

    double daily_lim = get_daily_limit_for_type(acc->type, config);
    double rem_daily_with = daily_lim - acc->daily_withdrawn;
    if (rem_daily_with < 0.0) rem_daily_with = 0.0;

    printf("Account Number         : %d\n", acc->account_number);
    printf("Account Type           : %s\n",
           acc->type == ACCOUNT_SAVINGS ? "Savings Account" :
           acc->type == ACCOUNT_CURRENT ? "Current / Business Account" : "Student Account");
    printf("Account Status         : %s\n",
           acc->status == STATUS_ACTIVE ? "Active & Healthy" :
           acc->status == STATUS_BLOCKED ? "BLOCKED (PIN Lockout)" : "FROZEN (Fraud Review)");
    printf("--------------------------------------------------------------------------------\n");
    printf("Cleared Ledger Balance : BDT %12.2f\n", acc->balance);
    printf("Required Minimum Hold  : BDT %12.2f\n", min_req);
    printf("Available for Cash Out : BDT %12.2f\n", avail);
    printf("--------------------------------------------------------------------------------\n");
    printf("Daily Withdrawal Limit : BDT %12.2f\n", daily_lim);
    printf("Withdrawn Today        : BDT %12.2f\n", acc->daily_withdrawn);
    printf("Remaining Daily Quota  : BDT %12.2f\n", rem_daily_with);
    printf("Low Balance Alert Set  : BDT %12.2f\n", acc->low_balance_threshold);
    printf("--------------------------------------------------------------------------------\n");

    if (acc->balance < acc->low_balance_threshold) {
        printf("[!] Health Status: ATTENTION NEEDED (Balance below alert threshold)\n");
    } else {
        printf("[+] Health Status: EXCELLENT (All covenants satisfied)\n");
    }
}

/* --- A16: Recurring Payments --- */
void manage_recurring_payments(Account *acc) {
    if (!acc) return;
    print_header("RECURRING BILL INSTRUCTIONS");
    printf("1. View Active Instructions\n");
    printf("2. Create New Recurring Instruction\n");
    printf("3. Pause / Resume Instruction\n");
    printf("4. Cancel Instruction\n");
    printf("5. Return to Main Menu\n");

    int choice = 5;
    get_int_input("Choose option [1-5]: ", &choice, 1, 5);

    if (choice == 1) {
        printf("\nRecurring Bill Directives (%d/%d):\n", acc->recurring_count, MAX_RECURRING_BILLS);
        if (acc->recurring_count == 0) {
            printf("[i] No recurring payment instructions active.\n");
        } else {
            for (int i = 0; i < acc->recurring_count; i++) {
                RecurringPayment *r = &acc->recurring_payments[i];
                struct tm *t = localtime(&r->next_due_date);
                char dt[32] = "N/A";
                if (t) strftime(dt, sizeof(dt), "%Y-%m-%d", t);
                printf("%d. ID #%-4d | %-12s | BDT %-8.2f | Provider: %-10s | Next Due: %s | %s\n",
                       i + 1, r->instruction_id,
                       r->category == BILL_ELECTRICITY ? "Electricity" :
                       r->category == BILL_GAS ? "Gas" :
                       r->category == BILL_INTERNET ? "Internet" : "Mobile",
                       r->amount, r->biller_name, dt, r->is_active ? "ACTIVE" : "PAUSED");
            }
        }
    } else if (choice == 2) {
        if (acc->recurring_count >= MAX_RECURRING_BILLS) {
            printf("[!] Recurring instructions limit reached (%d max).\n", MAX_RECURRING_BILLS);
            return;
        }

        printf("Bill Category: 0=Electricity, 1=Gas, 2=Internet, 3=Mobile\n");
        int cat = 0;
        get_int_input("Select category [0-3]: ", &cat, 0, 3);

        char provider[32], meter[32];
        printf("Enter Service Provider Name: ");
        get_line_input(provider, sizeof(provider));
        printf("Enter Meter / Phone Number: ");
        get_line_input(meter, sizeof(meter));

        double amt = 0.0;
        get_double_input("Enter Monthly Payment Sum: BDT ", &amt, 50.0, 50000.0);

        RecurringPayment *r = &acc->recurring_payments[acc->recurring_count++];
        r->instruction_id = 700 + acc->recurring_count;
        r->category = (BillCategory)cat;
        strncpy(r->biller_name, provider[0] ? provider : "Provider", 31);
        strncpy(r->account_or_meter, meter[0] ? meter : "Acc", 31);
        r->amount = amt;
        r->next_due_date = time(NULL) + (30 * 86400LL); /* Due in 30 days */
        r->is_active = true;

        save_all_data(ACCOUNTS_FILE);
        printf("[+] Recurring instruction #%d scheduled successfully!\n", r->instruction_id);
    } else if (choice == 3) {
        if (acc->recurring_count == 0) return;
        int idx = 0;
        get_int_input("Enter instruction number to toggle [1-N]: ", &idx, 1, acc->recurring_count);
        acc->recurring_payments[idx - 1].is_active = !acc->recurring_payments[idx - 1].is_active;
        save_all_data(ACCOUNTS_FILE);
        printf("[+] Instruction status updated to: %s\n",
               acc->recurring_payments[idx - 1].is_active ? "ACTIVE" : "PAUSED");
    } else if (choice == 4) {
        if (acc->recurring_count == 0) return;
        int idx = 0;
        get_int_input("Enter instruction number to cancel [1-N]: ", &idx, 1, acc->recurring_count);
        for (int i = idx - 1; i < acc->recurring_count - 1; i++) {
            acc->recurring_payments[i] = acc->recurring_payments[i + 1];
        }
        acc->recurring_count--;
        save_all_data(ACCOUNTS_FILE);
        printf("[+] Recurring instruction cancelled.\n");
    }
}

void process_scheduled_recurring_bills(AtmContext *atm) {
    if (!atm || !g_accounts) return;
    print_header("PROCESS SCHEDULED RECURRING BILLS");
    time_t now = time(NULL);
    int processed = 0;

    for (int i = 0; i < g_account_count; i++) {
        Account *a = &g_accounts[i];
        if (a->status != STATUS_ACTIVE) continue;

        for (int j = 0; j < a->recurring_count; j++) {
            RecurringPayment *r = &a->recurring_payments[j];
            if (r->is_active && now >= r->next_due_date) {
                if (a->balance - r->amount >= 100.0) {
                    a->balance -= r->amount;
                    char ref[MAX_REF_LEN];
                    char desc[MAX_DESC_LEN];
                    snprintf(desc, sizeof(desc), "Recurring: %.25s (%.25s)", r->biller_name, r->account_or_meter);
                    record_transaction(a, TXN_BILL_PAYMENT, r->amount, 0.0, r->instruction_id, desc, ref);
                    r->next_due_date = now + (30 * 86400LL);
                    processed++;

                    char note[128];
                    snprintf(note, sizeof(note), "Auto-paid recurring bill BDT %.2f to %s", r->amount, r->biller_name);
                    add_notification(a, note);
                } else {
                    add_notification(a, "Recurring bill failed: Insufficient account funds.");
                }
            }
        }
    }

    save_all_data(ACCOUNTS_FILE);
    printf("[+] Processing complete. Executed %d due recurring instructions.\n", processed);
}

/* --- A17: Financial Reports & Analytics --- */
void generate_customer_analytics(const Account *acc, const BankConfig *config) {
    if (!acc || !config) return;
    print_header("FINANCIAL ANALYTICS & SPENDING REPORT");

    double total_dep = 0.0, total_with = 0.0, total_trans_out = 0.0, total_trans_in = 0.0, total_bills = 0.0;
    double max_txn = 0.0, sum_all = 0.0;
    int count = acc->txn_count;

    for (int i = 0; i < count; i++) {
        const Transaction *t = &acc->transactions[i];
        if (t->is_reversed) continue;

        if (t->amount > max_txn) max_txn = t->amount;
        sum_all += t->amount;

        switch (t->type) {
            case TXN_DEPOSIT: total_dep += t->amount; break;
            case TXN_WITHDRAWAL: total_with += t->amount; break;
            case TXN_TRANSFER_OUT: total_trans_out += t->amount; break;
            case TXN_TRANSFER_IN: total_trans_in += t->amount; break;
            case TXN_BILL_PAYMENT: total_bills += t->amount; break;
            default: break;
        }
    }

    double avg_txn = count > 0 ? (sum_all / count) : 0.0;
    double daily_lim = get_daily_limit_for_type(acc->type, config);

    printf("Customer Name          : %s (Acc #%d)\n", acc->name, acc->account_number);
    printf("Total Historical Txns  : %d\n", count);
    printf("--------------------------------------------------------------------------------\n");
    printf("Total Cash Deposited   : BDT %12.2f\n", total_dep);
    printf("Total Cash Withdrawn   : BDT %12.2f\n", total_with);
    printf("Total Transfers Sent   : BDT %12.2f\n", total_trans_out);
    printf("Total Transfers Recv   : BDT %12.2f\n", total_trans_in);
    printf("Total Utility Bills    : BDT %12.2f\n", total_bills);
    printf("--------------------------------------------------------------------------------\n");
    printf("Largest Single Txn     : BDT %12.2f\n", max_txn);
    printf("Average Transaction    : BDT %12.2f\n", avg_txn);
    printf("Daily Allowance Left   : BDT %12.2f / %.2f\n", daily_lim - acc->daily_withdrawn, daily_lim);
    printf("--------------------------------------------------------------------------------\n");
}

void generate_admin_financial_report(const AtmContext *atm) {
    if (!atm) return;
    print_header("GLOBAL BANKING & TERMINAL AUDIT REPORT");

    double total_bank_deposits = 0.0;
    int active_cnt = 0, blocked_cnt = 0, fraud_cnt = 0;
    int total_fds = 0;
    double total_fd_principal = 0.0;

    for (int i = 0; i < g_account_count; i++) {
        const Account *a = &g_accounts[i];
        total_bank_deposits += a->balance;
        if (a->status == STATUS_ACTIVE) active_cnt++;
        else if (a->status == STATUS_BLOCKED) blocked_cnt++;
        else if (a->status == STATUS_FROZEN_FRAUD) fraud_cnt++;

        for (int j = 0; j < a->fd_count; j++) {
            if (a->fixed_deposits[j].is_active) {
                total_fds++;
                total_fd_principal += a->fixed_deposits[j].principal;
            }
        }
    }

    int cash_val = get_total_cash_value(&atm->cash);

    printf("Total Registered Accounts : %d\n", g_account_count);
    printf("  - Active Accounts       : %d\n", active_cnt);
    printf("  - Blocked (Bad PIN)     : %d\n", blocked_cnt);
    printf("  - Frozen (Fraud Alert)  : %d\n", fraud_cnt);
    printf("--------------------------------------------------------------------------------\n");
    printf("Total Customer Balances   : BDT %15.2f\n", total_bank_deposits);
    printf("Active Fixed Deposits     : %d (Principal: BDT %.2f)\n", total_fds, total_fd_principal);
    printf("Total System Obligations  : BDT %15.2f\n", total_bank_deposits + total_fd_principal);
    printf("--------------------------------------------------------------------------------\n");
    printf("ATM Hardware Status       : %s\n", atm->operational_status == ATM_OPERATIONAL ? "OPERATIONAL" : "MAINTENANCE");
    printf("ATM Vault Cash Inventory  : BDT %15d\n", cash_val);
    printf("  - 1,000 BDT Notes       : %d notes (BDT %d)\n", atm->cash.notes_1000, atm->cash.notes_1000 * 1000);
    printf("  -   500 BDT Notes       : %d notes (BDT %d)\n", atm->cash.notes_500, atm->cash.notes_500 * 500);
    printf("  -   100 BDT Notes       : %d notes (BDT %d)\n", atm->cash.notes_100, atm->cash.notes_100 * 100);
    printf("Active Disputes Pending   : %d\n", atm->dispute_count);
    printf("--------------------------------------------------------------------------------\n");
}

/* --- A20: Customer Disputes --- */
void file_customer_dispute(Account *acc, AtmContext *atm) {
    if (!acc || !atm) return;
    print_header("FILE TRANSACTION DISPUTE");

    if (atm->dispute_count >= MAX_DISPUTES_GLOBAL) {
        printf("[!] Dispute filing system is currently at capacity.\n");
        return;
    }

    char ref[MAX_REF_LEN];
    printf("Enter Transaction Reference ID (e.g., TXN-...): ");
    get_line_input(ref, sizeof(ref));

    /* Verify transaction belongs to account */
    bool found = false;
    for (int i = 0; i < acc->txn_count; i++) {
        if (strcmp(acc->transactions[i].ref_id, ref) == 0) {
            found = true;
            break;
        }
    }

    if (!found) {
        printf("[!] Error: No transaction with reference '%s' found in your account history.\n", ref);
        return;
    }

    char reason[MAX_REASON_LEN];
    printf("State reason for dispute (e.g., wrong sum, terminal malfunction): ");
    get_line_input(reason, sizeof(reason));

    Dispute *d = &atm->disputes[atm->dispute_count++];
    d->dispute_id = 9000 + atm->dispute_count;
    strncpy(d->txn_ref, ref, MAX_REF_LEN - 1);
    d->account_number = acc->account_number;
    strncpy(d->reason, reason[0] ? reason : "Unspecified discrepancy", MAX_REASON_LEN - 1);
    d->filed_at = time(NULL);
    d->status = DISPUTE_OPEN;
    d->resolution_note[0] = '\0';
    d->resolved_by_admin = 0;
    d->resolved_at = 0;

    char log_buf[256];
    snprintf(log_buf, sizeof(log_buf), "Dispute Filed: Case #%d on Txn %.30s | Reason: %.120s", d->dispute_id, ref, d->reason);
    audit_log("DISPUTE_FILED", acc->account_number, log_buf);

    save_all_data(ACCOUNTS_FILE);
    printf("\n[+] SUCCESS: Dispute Case #%d registered. Status: OPEN.\n", d->dispute_id);
    printf("[i] Note: Filing a dispute does NOT automatically reverse transactions. Administrative review required.\n");
}

void manage_admin_disputes(AtmContext *atm) {
    if (!atm) return;
    print_header("MANAGE CUSTOMER DISPUTES");

    if (atm->dispute_count == 0) {
        printf("[i] No customer disputes currently registered.\n");
        return;
    }

    for (int i = 0; i < atm->dispute_count; i++) {
        Dispute *d = &atm->disputes[i];
        printf("%d. Case #%-4d | Acc #%-6d | Txn: %-26s | Status: %s | Reason: %s\n",
               i + 1, d->dispute_id, d->account_number, d->txn_ref,
               d->status == DISPUTE_OPEN ? "OPEN" :
               d->status == DISPUTE_UNDER_REVIEW ? "UNDER REVIEW" :
               d->status == DISPUTE_RESOLVED ? "RESOLVED" : "REJECTED",
               d->reason);
    }

    printf("\nSelect case number to adjudicate [1-N, 0 to exit]: ");
    int sel = 0;
    get_int_input("", &sel, 0, atm->dispute_count);
    if (sel == 0) return;

    Dispute *target = &atm->disputes[sel - 1];
    printf("\nUpdating Case #%d (Txn: %s)\n", target->dispute_id, target->txn_ref);
    printf("1. Set Status: UNDER REVIEW\n");
    printf("2. Set Status: RESOLVED\n");
    printf("3. Set Status: REJECTED\n");
    int st = 1;
    get_int_input("Choose action [1-3]: ", &st, 1, 3);

    char note[MAX_NOTE_LEN];
    printf("Enter official resolution finding / note: ");
    get_line_input(note, sizeof(note));

    target->status = (st == 1 ? DISPUTE_UNDER_REVIEW : (st == 2 ? DISPUTE_RESOLVED : DISPUTE_REJECTED));
    strncpy(target->resolution_note, note[0] ? note : "Review concluded", MAX_NOTE_LEN - 1);
    target->resolved_by_admin = 9999;
    target->resolved_at = time(NULL);

    char log_buf[256];
    snprintf(log_buf, sizeof(log_buf), "Dispute #%d status updated to %d | Note: %.120s", target->dispute_id, target->status, note);
    audit_log("DISPUTE_UPDATE", target->account_number, log_buf);

    Account *acc = find_account(target->account_number);
    if (acc) {
        char cust_msg[128];
        snprintf(cust_msg, sizeof(cust_msg), "Dispute Case #%d status updated to %s",
                 target->dispute_id,
                 target->status == DISPUTE_RESOLVED ? "RESOLVED" :
                 target->status == DISPUTE_REJECTED ? "REJECTED" : "UNDER REVIEW");
        add_notification(acc, cust_msg);
    }

    save_all_data(ACCOUNTS_FILE);
    printf("[+] Case #%d updated successfully!\n", target->dispute_id);
}

/* --- A21: PIN Change & Reset --- */
bool customer_change_pin(Account *acc) {
    if (!acc) return false;
    print_header("CHANGE 4-DIGIT PIN");

    char old_pin[MAX_PIN_LEN];
    printf("Enter current 4-digit PIN: ");
    get_line_input(old_pin, sizeof(old_pin));

    if (!verify_pin(old_pin, acc->pin_salt, acc->pin_hash)) {
        printf("[!] Error: Current PIN verification failed.\n");
        audit_log("PIN_CHANGE_FAIL", acc->account_number, "Failed old PIN verification");
        return false;
    }

    char new_pin1[MAX_PIN_LEN], new_pin2[MAX_PIN_LEN];
    printf("Enter new 4-digit PIN: ");
    get_line_input(new_pin1, sizeof(new_pin1));
    if (strlen(new_pin1) != 4 || !isdigit((unsigned char)new_pin1[0]) ||
        !isdigit((unsigned char)new_pin1[1]) || !isdigit((unsigned char)new_pin1[2]) ||
        !isdigit((unsigned char)new_pin1[3])) {
        printf("[!] Error: PIN must be exactly 4 numeric digits.\n");
        return false;
    }

    printf("Confirm new 4-digit PIN: ");
    get_line_input(new_pin2, sizeof(new_pin2));
    if (strcmp(new_pin1, new_pin2) != 0) {
        printf("[!] Error: New PIN entries do not match.\n");
        return false;
    }

    /* Generate fresh salt on each PIN change */
    generate_salt(acc->pin_salt, 16);
    compute_pin_hash(new_pin1, acc->pin_salt, acc->pin_hash);
    acc->must_change_pin = false;

    save_all_data(ACCOUNTS_FILE);
    audit_log("PIN_CHANGE_OK", acc->account_number, "Customer successfully updated PIN");

    printf("\n[+] SUCCESS: Your PIN has been updated securely!\n");
    return true;
}

bool admin_reset_customer_pin(Account *acc) {
    if (!acc) return false;
    printf("\nResetting PIN for Customer Acc #%d (%s)\n", acc->account_number, acc->name);

    char temp_pin[MAX_PIN_LEN];
    printf("Enter temporary 4-digit PIN: ");
    get_line_input(temp_pin, sizeof(temp_pin));
    if (strlen(temp_pin) != 4) {
        printf("[!] Error: Temporary PIN must be exactly 4 digits.\n");
        return false;
    }

    generate_salt(acc->pin_salt, 16);
    compute_pin_hash(temp_pin, acc->pin_salt, acc->pin_hash);
    acc->must_change_pin = true; /* Force immediate change on next login per A21 */
    acc->failed_pin_attempts = 0;
    acc->status = STATUS_ACTIVE;

    save_all_data(ACCOUNTS_FILE);
    audit_log("PIN_ADMIN_RESET", acc->account_number, "Admin set temporary PIN; mandatory change flagged");

    printf("[+] Temporary PIN assigned. Customer MUST change it immediately upon next login.\n");
    return true;
}

/* ========================================================================== */
/*                             MENU INTERFACES                                */
/* ========================================================================== */

void customer_menu(Account *acc, AtmContext *atm) {
    if (!acc || !atm) return;

    /* A21: Check forced PIN change */
    if (acc->must_change_pin) {
        printf("\n[!] ATTENTION: Your account is configured with a temporary PIN.\n");
        printf("[!] Security Policy (A21) mandates changing your PIN before continuing.\n");
        if (!customer_change_pin(acc)) {
            printf("[!] PIN update required. Logging out.\n");
            return;
        }
    }

    while (1) {
        if (check_session_timeout(atm)) return;

        print_header("CUSTOMER SERVICES PORTAL");
        printf("Customer: %-25s | Account #%-6d | Status: %s\n",
               acc->name, acc->account_number,
               acc->status == STATUS_ACTIVE ? "Active" :
               acc->status == STATUS_BLOCKED ? "BLOCKED" : "FROZEN");
        printf("================================================================================\n");
        printf(" 1. Balance Inquiry & Receipt       8. Beneficiary Management (A11)\n");
        printf(" 2. Cash Withdrawal                 9. Statement Search & CSV Export (A9)\n");
        printf(" 3. Cash Deposit                   10. Multi-Currency Balances (A6)\n");
        printf(" 4. Fund Transfer                  11. Fixed Deposit & Loan EMI (A5)\n");
        printf(" 5. Utility Bill Payment           12. Recurring Bill Instructions (A16)\n");
        printf(" 6. Mini Statement (Last 10)       13. Account Health Summary (A15)\n");
        printf(" 7. Change PIN                     14. Notifications (%d) & Dispute (A14, A20)\n", acc->notification_count);
        printf(" 0. Logout & Return to Welcome Screen\n");
        printf("================================================================================\n");

        int choice = 0;
        if (!get_int_input("Select an option [0-14]: ", &choice, 0, 14)) {
            if (feof(stdin)) break;
            continue;
        }
        touch_session(atm);

        if (choice == 0) {
            printf("[*] Logging out. Have a nice day!\n");
            audit_log("LOGOUT", acc->account_number, "Customer ended session");
            break;
        }

        /* Prevent restricted actions on frozen/blocked accounts */
        if ((acc->status != STATUS_ACTIVE) && (choice == 2 || choice == 3 || choice == 4 || choice == 5 || choice == 11)) {
            printf("\n[!] ACCESS DENIED: Account is currently %s.\n",
                   acc->status == STATUS_BLOCKED ? "BLOCKED (PIN Lockout)" : "FROZEN (Fraud Protection)");
            printf("[!] Financial transactions are suspended. Contact bank administration.\n");
            press_enter_to_continue();
            continue;
        }

        switch (choice) {
            case 1: { /* Balance Inquiry */
                print_header("BALANCE INQUIRY");
                printf("Account Number : %d\n", acc->account_number);
                printf("Account Holder : %s\n", acc->name);
                printf("Cleared Balance: BDT %.2f\n", acc->balance);
                printf("Print paper receipt? (1=Yes, 2=No): ");
                int r = 2;
                get_int_input("", &r, 1, 2);
                if (r == 1) {
                    Transaction dummy;
                    memset(&dummy, 0, sizeof(dummy));
                    generate_reference_id(dummy.ref_id, sizeof(dummy.ref_id));
                    dummy.type = TXN_DEPOSIT;
                    dummy.amount = 0.0;
                    dummy.fee = 0.0;
                    dummy.balance_after = acc->balance;
                    dummy.timestamp = time(NULL);
                    strncpy(dummy.description, "Balance Inquiry Slip", MAX_DESC_LEN - 1);
                    print_receipt(&dummy, acc);
                }
                break;
            }
            case 2: { /* Cash Withdrawal */
                double amt = 0.0;
                printf("\nStandard Amounts: 500, 1000, 2000, 5000, 10000, 20000\n");
                get_double_input("Enter withdrawal amount (multiples of 100): BDT ", &amt, 100.0, 100000.0);
                perform_withdrawal(acc, amt, atm);
                break;
            }
            case 3: { /* Cash Deposit */
                double amt = 0.0;
                get_double_input("Enter cash deposit amount: BDT ", &amt, 100.0, 500000.0);
                perform_deposit(acc, amt);
                break;
            }
            case 4: { /* Fund Transfer */
                int dest_acc_num = 0;
                printf("\n1. Transfer to Saved Beneficiary\n");
                printf("2. Transfer to New Account Number\n");
                int t_mode = 2;
                get_int_input("Select transfer mode [1-2]: ", &t_mode, 1, 2);

                if (t_mode == 1) {
                    if (acc->beneficiary_count == 0) {
                        printf("[i] No saved beneficiaries. Please enter account number manually.\n");
                        get_int_input("Enter Destination Account Number: ", &dest_acc_num, 1000, 999999);
                    } else {
                        for (int i = 0; i < acc->beneficiary_count; i++) {
                            printf("%d. #%-6d (%s)\n", i + 1, acc->beneficiaries[i].account_number, acc->beneficiaries[i].name);
                        }
                        int b_idx = 1;
                        get_int_input("Choose beneficiary: ", &b_idx, 1, acc->beneficiary_count);
                        dest_acc_num = acc->beneficiaries[b_idx - 1].account_number;
                    }
                } else {
                    get_int_input("Enter Destination Account Number: ", &dest_acc_num, 1000, 999999);
                }

                Account *dest = find_account(dest_acc_num);
                if (!dest) {
                    printf("[!] Error: Destination account #%d not found.\n", dest_acc_num);
                } else {
                    printf("\nRecipient Verified: >> %s << (Acc #%d)\n", dest->name, dest->account_number);
                    double amt = 0.0;
                    get_double_input("Enter transfer amount: BDT ", &amt, 10.0, 500000.0);
                    printf("Confirm transfer of BDT %.2f to %s? (1=Confirm, 2=Cancel): ", amt, dest->name);
                    int conf = 2;
                    get_int_input("", &conf, 1, 2);
                    if (conf == 1) {
                        perform_transfer(acc, dest, amt, atm);
                    } else {
                        printf("[*] Transfer cancelled.\n");
                    }
                }
                break;
            }
            case 5: { /* Utility Bill Payment */
                print_header("UTILITY BILL PAYMENT");
                printf("1. Electricity Bill (DESCO / DPDC / NESCO)\n");
                printf("2. Gas Bill (Titas / Karnaphuli)\n");
                printf("3. Internet Bill (Broadband Provider)\n");
                printf("4. Mobile Top-Up (GP / Robi / Banglalink / Airtel)\n");
                int b_opt = 1;
                get_int_input("Select bill category [1-4]: ", &b_opt, 1, 4);

                char biller[32], ref_no[32];
                printf("Enter Provider Name: ");
                get_line_input(biller, sizeof(biller));
                printf("Enter Meter / Account / Phone Number: ");
                get_line_input(ref_no, sizeof(ref_no));

                double amt = 0.0;
                get_double_input("Enter bill amount: BDT ", &amt, 10.0, 50000.0);
                perform_bill_payment(acc, (BillCategory)(b_opt - 1), biller, ref_no, amt);
                break;
            }
            case 6: { /* Mini Statement (Last 10) */
                print_header("MINI STATEMENT (LAST 10 TRANSACTIONS)");
                if (acc->txn_count == 0) {
                    printf("[i] No transactions recorded.\n");
                } else {
                    int start = acc->txn_count > 10 ? acc->txn_count - 10 : 0;
                    printf("%-26s | %-19s | %-12s | %10s | %10s\n", "Reference ID", "Date Time", "Type", "Amount", "Balance");
                    printf("-----------------------------------------------------------------------------------------\n");
                    for (int i = acc->txn_count - 1; i >= start; i--) {
                        Transaction *t = &acc->transactions[i];
                        struct tm *tm_p = localtime(&t->timestamp);
                        char dt[32] = "N/A";
                        if (tm_p) strftime(dt, sizeof(dt), "%Y-%m-%d %H:%M", tm_p);
                        printf("%-26s | %-19s | %-12s | %10.2f | %10.2f\n",
                               t->ref_id, dt,
                               t->type == TXN_DEPOSIT ? "Deposit" :
                               t->type == TXN_WITHDRAWAL ? "Withdrawal" :
                               t->type == TXN_TRANSFER_OUT ? "Transfer Out" :
                               t->type == TXN_TRANSFER_IN ? "Transfer In" :
                               t->type == TXN_BILL_PAYMENT ? "Bill Pay" : "Reversal",
                               t->amount, t->balance_after);
                    }
                }
                break;
            }
            case 7: { /* Change PIN */
                customer_change_pin(acc);
                break;
            }
            case 8: { /* Beneficiary Management */
                manage_beneficiaries(acc);
                break;
            }
            case 9: { /* Statement Tools */
                search_and_export_statements(acc);
                break;
            }
            case 10: { /* Multi-Currency */
                display_multi_currency_balances(acc, &atm->rates);
                break;
            }
            case 11: { /* Fixed Deposit & Loan EMI */
                printf("\n1. Open Fixed Deposit\n2. Loan EMI Amortization Calculator\n");
                int fd_opt = 1;
                get_int_input("Choose option [1-2]: ", &fd_opt, 1, 2);
                if (fd_opt == 1) {
                    double p = 0.0;
                    int m = 12;
                    get_double_input("Enter Fixed Deposit Principal: BDT ", &p, 1000.0, 10000000.0);
                    get_int_input("Enter Term Duration in Months [3-60]: ", &m, 3, 60);
                    open_fixed_deposit(acc, p, m, 8.50);
                } else {
                    double lp = 0.0, lr = 9.50;
                    int lm = 24;
                    get_double_input("Enter Loan Principal: BDT ", &lp, 10000.0, 50000000.0);
                    get_double_input("Enter Annual Interest Rate (%): ", &lr, 0.0, 30.0);
                    get_int_input("Enter Repayment Term in Months [6-120]: ", &lm, 6, 120);
                    calculate_loan_emi_schedule(lp, lr, lm);
                }
                break;
            }
            case 12: { /* Recurring Payments */
                manage_recurring_payments(acc);
                break;
            }
            case 13: { /* Account Health */
                display_account_health(acc, &atm->config);
                break;
            }
            case 14: { /* Notifications & Disputes */
                printf("\n1. View Notifications\n2. File a Transaction Dispute\n");
                int nd_opt = 1;
                get_int_input("Choose [1-2]: ", &nd_opt, 1, 2);
                if (nd_opt == 1) view_notifications(acc);
                else file_customer_dispute(acc, atm);
                break;
            }
            default:
                break;
        }

        press_enter_to_continue();
    }
}

void admin_menu(AtmContext *atm) {
    if (!atm) return;

    print_header("ADMINISTRATOR CONTROL CONSOLE");
    char pass[MAX_PIN_LEN];
    printf("Enter Administrator Security PIN: ");
    get_line_input(pass, sizeof(pass));

    if (!verify_pin(pass, atm->config.admin_pin_salt, atm->config.admin_pin_hash)) {
        printf("[!] ACCESS DENIED: Invalid administrator security PIN.\n");
        audit_log("ADMIN_LOGIN_FAIL", 0, "Invalid admin PIN attempt");
        press_enter_to_continue();
        return;
    }

    audit_log("ADMIN_LOGIN_OK", 0, "Administrator session initialized");

    while (1) {
        print_header("ADMINISTRATIVE MANAGEMENT CONSOLE");
        printf("ATM Operational Status : %s\n", atm->operational_status == ATM_OPERATIONAL ? "OPERATIONAL" : "MAINTENANCE");
        printf("Vault Cash Available   : BDT %d\n", get_total_cash_value(&atm->cash));
        printf("================================================================================\n");
        printf(" 1. View All Customer Accounts      8. Reversal of Financial Txn (A13)\n");
        printf(" 2. Create New Customer Account     9. Adjudicate Disputes (A20)\n");
        printf(" 3. Block Customer Account         10. Refill Cash Inventory (A10, A22)\n");
        printf(" 4. Unblock / Unfreeze Account     11. Toggle Maintenance Mode (A22)\n");
        printf(" 5. Delete Customer Account        12. Global Bank Analytics Report (A17)\n");
        printf(" 6. Reset Customer PIN (A21)       13. Process Scheduled Recurring Bills (A16)\n");
        printf(" 7. Run End of Month Cycle (A4)    14. Update Foreign Exchange Rates (A6)\n");
        printf(" 0. Exit Administrator Mode\n");
        printf("================================================================================\n");

        int choice = 0;
        if (!get_int_input("Select administrative option [0-14]: ", &choice, 0, 14)) {
            if (feof(stdin)) break;
            continue;
        }

        if (choice == 0) {
            printf("[*] Exiting Administrator console.\n");
            audit_log("ADMIN_LOGOUT", 0, "Admin terminated session");
            break;
        }

        switch (choice) {
            case 1: { /* View All Accounts */
                print_header("ALL REGISTERED ACCOUNTS");
                printf("%-6s | %-20s | %-10s | %-12s | %12s\n", "Acc #", "Holder Name", "Type", "Status", "Balance (BDT)");
                printf("----------------------------------------------------------------------\n");
                for (int i = 0; i < g_account_count; i++) {
                    Account *a = &g_accounts[i];
                    printf("%-6d | %-20s | %-10s | %-12s | %12.2f\n",
                           a->account_number, a->name,
                           a->type == ACCOUNT_SAVINGS ? "Savings" :
                           a->type == ACCOUNT_CURRENT ? "Current" : "Student",
                           a->status == STATUS_ACTIVE ? "ACTIVE" :
                           a->status == STATUS_BLOCKED ? "BLOCKED" : "FROZEN",
                           a->balance);
                }
                break;
            }
            case 2: { /* Create Account */
                print_header("CREATE NEW ACCOUNT");
                int acc_num = 0;
                get_int_input("Enter new unique Account Number: ", &acc_num, 1000, 999999);
                if (find_account(acc_num)) {
                    printf("[!] Error: Account #%d already exists.\n", acc_num);
                    break;
                }
                char name[MAX_NAME_LEN], pin[MAX_PIN_LEN];
                printf("Enter Customer Legal Name: ");
                get_line_input(name, sizeof(name));
                printf("Enter Initial 4-digit PIN: ");
                get_line_input(pin, sizeof(pin));
                printf("Select Account Type (0=Savings, 1=Current, 2=Student): ");
                int t = 0;
                get_int_input("", &t, 0, 2);
                double dep = 0.0;
                get_double_input("Enter Initial Deposit Sum: BDT ", &dep, 100.0, 10000000.0);

                Account *new_acc = create_account(acc_num, name, pin, (AccountType)t, dep);
                if (new_acc) {
                    char ref[MAX_REF_LEN];
                    record_transaction(new_acc, TXN_DEPOSIT, dep, 0.0, 0, "Account Opening Deposit", ref);
                    save_all_data(ACCOUNTS_FILE);
                    char log_buf[128];
                    snprintf(log_buf, sizeof(log_buf), "Admin created account #%d (%s)", acc_num, name);
                    audit_log("ACC_CREATE", acc_num, log_buf);
                    printf("[+] SUCCESS: Account #%d successfully created!\n", acc_num);
                }
                break;
            }
            case 3: { /* Block Account */
                int acc_num = 0;
                get_int_input("Enter Account Number to BLOCK: ", &acc_num, 1000, 999999);
                Account *a = find_account(acc_num);
                if (a) {
                    a->status = STATUS_BLOCKED;
                    save_all_data(ACCOUNTS_FILE);
                    audit_log("ACC_BLOCK", acc_num, "Admin manually blocked account");
                    printf("[+] Account #%d is now BLOCKED.\n", acc_num);
                } else {
                    printf("[!] Account #%d not found.\n", acc_num);
                }
                break;
            }
            case 4: { /* Unblock / Unfreeze */
                int acc_num = 0;
                get_int_input("Enter Account Number to UNBLOCK / UNFREEZE: ", &acc_num, 1000, 999999);
                Account *a = find_account(acc_num);
                if (a) {
                    a->status = STATUS_ACTIVE;
                    a->failed_pin_attempts = 0;
                    a->failed_otp_attempts = 0;
                    save_all_data(ACCOUNTS_FILE);
                    audit_log("ACC_UNBLOCK", acc_num, "Admin unblocked/unfroze account");
                    printf("[+] Account #%d restored to ACTIVE status.\n", acc_num);
                } else {
                    printf("[!] Account #%d not found.\n", acc_num);
                }
                break;
            }
            case 5: { /* Delete Account */
                int acc_num = 0;
                get_int_input("Enter Account Number to DELETE: ", &acc_num, 1000, 999999);
                printf("Are you sure you want to permanently delete Account #%d? (1=Yes, 2=No): ", acc_num);
                int conf = 2;
                get_int_input("", &conf, 1, 2);
                if (conf == 1) {
                    if (delete_account(acc_num)) {
                        save_all_data(ACCOUNTS_FILE);
                        audit_log("ACC_DELETE", acc_num, "Admin deleted account");
                        printf("[+] Account #%d deleted.\n", acc_num);
                    } else {
                        printf("[!] Account #%d not found.\n", acc_num);
                    }
                }
                break;
            }
            case 6: { /* Reset PIN */
                int acc_num = 0;
                get_int_input("Enter Account Number to Reset PIN: ", &acc_num, 1000, 999999);
                Account *a = find_account(acc_num);
                if (a) admin_reset_customer_pin(a);
                else printf("[!] Account #%d not found.\n", acc_num);
                break;
            }
            case 7: { /* End of Month */
                process_end_of_month(atm);
                break;
            }
            case 8: { /* Reversal */
                char target_ref[MAX_REF_LEN];
                printf("Enter Transaction Reference ID to REVERSE: ");
                get_line_input(target_ref, sizeof(target_ref));
                char reason[MAX_REASON_LEN];
                printf("Enter reason for reversal: ");
                get_line_input(reason, sizeof(reason));
                printf("Confirm administrative reversal of %s? (1=Confirm, 2=Cancel): ", target_ref);
                int conf = 2;
                get_int_input("", &conf, 1, 2);
                if (conf == 1) perform_reversal(target_ref, reason, 9999, atm);
                break;
            }
            case 9: { /* Disputes */
                manage_admin_disputes(atm);
                break;
            }
            case 10: { /* Refill Cash */
                print_header("REFILL ATM CASH VAULT");
                printf("Current Stock: 1000s: %d | 500s: %d | 100s: %d (Total BDT %d)\n",
                       atm->cash.notes_1000, atm->cash.notes_500, atm->cash.notes_100, get_total_cash_value(&atm->cash));
                int a1000 = 0, a500 = 0, a100 = 0;
                get_int_input("Add 1000 BDT Notes count: ", &a1000, 0, 5000);
                get_int_input("Add  500 BDT Notes count: ", &a500, 0, 5000);
                get_int_input("Add  100 BDT Notes count: ", &a100, 0, 5000);
                refill_cash(&atm->cash, a1000, a500, a100);
                save_all_data(ACCOUNTS_FILE);
                char log_buf[128];
                snprintf(log_buf, sizeof(log_buf), "Admin refilled vault: +%d(1k), +%d(500), +%d(100)", a1000, a500, a100);
                audit_log("CASH_REFILL", 0, log_buf);
                printf("[+] Cash refilled successfully! New Vault Total: BDT %d\n", get_total_cash_value(&atm->cash));
                break;
            }
            case 11: { /* Toggle Maintenance Mode */
                atm->operational_status = (atm->operational_status == ATM_OPERATIONAL ? ATM_MAINTENANCE : ATM_OPERATIONAL);
                save_all_data(ACCOUNTS_FILE);
                char log_buf[64];
                snprintf(log_buf, sizeof(log_buf), "ATM status changed to %s",
                         atm->operational_status == ATM_OPERATIONAL ? "OPERATIONAL" : "MAINTENANCE");
                audit_log("ATM_MODE", 0, log_buf);
                printf("[+] ATM mode toggled to: %s\n", atm->operational_status == ATM_OPERATIONAL ? "OPERATIONAL" : "MAINTENANCE");
                break;
            }
            case 12: { /* Global Report */
                generate_admin_financial_report(atm);
                break;
            }
            case 13: { /* Recurring Bills */
                process_scheduled_recurring_bills(atm);
                break;
            }
            case 14: { /* FX Rates */
                print_header("UPDATE FOREIGN EXCHANGE RATES");
                printf("Current: 1 USD = %.2f BDT | 1 EUR = %.2f BDT\n", atm->rates.bdt_per_usd, atm->rates.bdt_per_eur);
                double nu = 0.0, ne = 0.0;
                get_double_input("Enter new USD to BDT rate: ", &nu, 1.0, 500.0);
                get_double_input("Enter new EUR to BDT rate: ", &ne, 1.0, 500.0);
                atm->rates.bdt_per_usd = nu;
                atm->rates.bdt_per_eur = ne;
                save_rates_file(RATES_FILE, &atm->rates);
                audit_log("RATES_UPDATE", 0, "Admin updated foreign exchange rates");
                printf("[+] Rates updated successfully.\n");
                break;
            }
            default:
                break;
        }

        press_enter_to_continue();
    }
}

void main_atm_loop(AtmContext *atm) {
    if (!atm) return;
    set_atm_context_ref(atm);

    while (1) {
        print_header("ANTIGRAVITY BANKING TERMINAL");
        printf("Operational Mode: %s | Base Currency: BDT (৳)\n",
               atm->operational_status == ATM_OPERATIONAL ? "OPERATIONAL" : "MAINTENANCE");
        printf("================================================================================\n");
        printf(" 1. Customer Card & PIN Login\n");
        printf(" 2. Administrator Access Mode\n");
        printf(" 3. Shutdown System\n");
        printf("================================================================================\n");

        int choice = 0;
        if (!get_int_input("Please select [1-3]: ", &choice, 1, 3)) {
            if (feof(stdin)) break;
            continue;
        }

        if (choice == 3) {
            printf("\n[*] Gracefully saving data and shutting down ATM terminal...\n");
            save_all_data(ACCOUNTS_FILE);
            audit_log("SHUTDOWN", 0, "ATM terminal cleanly halted by operator");
            printf("[+] Shutdown complete. Goodbye!\n");
            break;
        }

        if (choice == 2) {
            admin_menu(atm);
            continue;
        }

        /* Customer Login Flow */
        print_header("CUSTOMER SECURE LOGIN");
        int acc_num = 0;
        get_int_input("Enter your 4-digit Account Number (e.g. 1001): ", &acc_num, 1000, 999999);

        Account *acc = find_account(acc_num);
        if (!acc) {
            printf("[!] Error: Account #%d does not exist.\n", acc_num);
            audit_log("LOGIN_FAIL", acc_num, "Non-existent account number");
            press_enter_to_continue();
            continue;
        }

        if (acc->status == STATUS_BLOCKED) {
            printf("\n[!] CARD BLOCKED: 3 consecutive wrong PIN attempts entered.\n");
            printf("[!] Please visit an Antigravity branch or contact bank admin to unblock.\n");
            audit_log("LOGIN_BLOCKED", acc->account_number, "Rejected login attempt on blocked card");
            press_enter_to_continue();
            continue;
        }

        char pin[MAX_PIN_LEN];
        printf("Enter your 4-digit Secret PIN: ");
        get_line_input(pin, sizeof(pin));

        if (!verify_pin(pin, acc->pin_salt, acc->pin_hash)) {
            acc->failed_pin_attempts++;
            printf("\n[-] Invalid PIN! Failed attempt %d of %d.\n",
                   acc->failed_pin_attempts, atm->config.max_pin_attempts);

            char log_buf[64];
            snprintf(log_buf, sizeof(log_buf), "Failed PIN attempt (%d/%d)", acc->failed_pin_attempts, atm->config.max_pin_attempts);
            audit_log("LOGIN_BAD_PIN", acc->account_number, log_buf);

            if (acc->failed_pin_attempts >= atm->config.max_pin_attempts) {
                acc->status = STATUS_BLOCKED;
                printf("\n[!] CARD BLOCKED: You have exceeded the maximum allowed attempts.\n");
                printf("[!] For your safety, the card is blocked until an administrator unblocks it.\n");
                audit_log("CARD_BLOCKED", acc->account_number, "Card locked due to 3 bad PIN attempts");
            }
            save_all_data(ACCOUNTS_FILE);
            press_enter_to_continue();
            continue;
        }

        /* Successful authentication */
        acc->failed_pin_attempts = 0;
        touch_session(atm);
        audit_log("LOGIN_SUCCESS", acc->account_number, "Customer authentication successful");
        printf("\n[+] Authentication Successful. Welcome, %s!\n", acc->name);

        customer_menu(acc, atm);
    }
}
