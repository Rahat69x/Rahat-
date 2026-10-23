/**
 * include/types.h
 * Single canonical definition of all domain models, enums, and structures (Section 4.2).
 * ISO C11 standard. All money stored in int64_t minor units (C4).
 */

#ifndef TYPES_H
#define TYPES_H

#include <stdint.h>
#include <stdbool.h>
#include <time.h>

#define MAX_NAME_LEN 48
#define MAX_NOTE_LEN 40
#define RECEIPT_ID_LEN 24
#define HASH_SIZE 32
#define SALT_SIZE 16

#define MAX_ACCOUNTS 100
#define MAX_TXNS_TOTAL 5000
#define MAX_ADMINS 16
#define MAX_LOANS 64
#define MAX_VOUCHERS 128
#define MAX_STANDING_ORDERS 64
#define MAX_ALERTS 128
#define MAX_SESSIONS 16
#define MAX_AUDIT_ENTRIES 2000

/* Status enums */
typedef enum {
    STATUS_ACTIVE = 0,
    STATUS_BLOCKED = 1,
    STATUS_FROZEN = 2,
    STATUS_CLOSED = 3
} AccountStatus;

typedef enum {
    TXN_DEPOSIT = 0,
    TXN_WITHDRAW = 1,
    TXN_TRANSFER_OUT = 2,
    TXN_TRANSFER_IN = 3,
    TXN_LOAN_DISBURSE = 4,
    TXN_LOAN_EMI = 5,
    TXN_FEE = 6,
    TXN_INTEREST = 7,
    TXN_VOUCHER = 8,
    TXN_STANDING_ORDER = 9
} TxnType;

typedef enum {
    ROLE_SUPERADMIN = 0,
    ROLE_ADMIN = 1,
    ROLE_AUDITOR = 2
} AdminRole;

typedef enum {
    LOAN_PERSONAL = 0,
    LOAN_SHORT = 1
} LoanType;

typedef enum {
    LOAN_PENDING = 0,
    LOAN_ACTIVE = 1,
    LOAN_PAID_OFF = 2,
    LOAN_OVERDUE = 3,
    LOAN_REJECTED = 4
} LoanStatus;

typedef enum {
    VOUCHER_CREDIT = 0,
    VOUCHER_CASHBACK = 1,
    VOUCHER_FEE_WAIVER = 2
} VoucherType;

typedef enum {
    VOUCHER_ACTIVE = 0,
    VOUCHER_USED_UP = 1,
    VOUCHER_EXPIRED = 2,
    VOUCHER_VOID = 3
} VoucherStatus;

typedef enum {
    FREQ_DAILY = 1,
    FREQ_WEEKLY = 7,
    FREQ_MONTHLY = 30
} Frequency;

typedef enum {
    ALERT_NEW = 0,
    ALERT_ACKED = 1
} AlertStatus;

/* Configuration loaded from config.ini (Section 3) */
typedef struct {
    char currency[8];
    int64_t withdraw_multiple;
    int64_t per_withdraw_limit;
    int64_t daily_withdraw_limit;
    int max_pin_tries;
    int pin_hash_iterations;
    int otp_ttl_s;
    int otp_max_tries;
    int otp_resend_max;
    int otp_resend_gap_s;
    int64_t otp_transfer_threshold;
    int fraud_warn;
    int fraud_stepup;
    int fraud_freeze;
    int admin_max_tries;
    int64_t loan_cap;
    int64_t loan_dual_approval_limit;
    int loan_max_active;
    int loan_min_account_age_days;
    int late_fee_pct;
    int savings_rate_pct;
    int voucher_bad_tries;
    char server_bind[64];
    int server_port;
    int server_max_clients;
    int session_idle_s;
} Config;

/* Transaction Record */
typedef struct {
    uint64_t id;
    long account;
    int day;
    time_t clock_time;
    TxnType type;
    int64_t amount;           /* Minor units */
    int64_t balance_after;    /* Minor units */
    long counterparty;
    char note[MAX_NOTE_LEN + 1];
    char receipt_no[RECEIPT_ID_LEN + 1];
} Txn;

/* Customer Account */
typedef struct {
    long id;
    char holder_name[MAX_NAME_LEN + 1];
    uint8_t pin_salt[SALT_SIZE];
    uint8_t pin_hash[HASH_SIZE];
    int pin_iter;
    bool must_change_pin;
    int64_t balance;          /* Minor units */
    int64_t withdrawn_today;  /* Minor units */
    int last_withdraw_day;
    AccountStatus status;
    int failed_pins;
    int risk_score;
    int64_t interest_remainder;
    int created_day;
    int64_t per_withdraw_override;
    int64_t daily_withdraw_override;
    int recent_op_count;
    time_t recent_op_times[8];
} Account;

/* Administrator */
typedef struct {
    char id[16];
    char name[MAX_NAME_LEN + 1];
    uint8_t passphrase_salt[SALT_SIZE];
    uint8_t passphrase_hash[HASH_SIZE];
    AdminRole role;
    bool is_locked;
    int failed_attempts;
    bool must_change_pass;
} Admin;

/* Loan Product */
typedef struct {
    uint64_t id;
    long account_id;
    LoanType type;
    int64_t principal;
    int interest_rate_pct;
    int tenure_months;
    int64_t emi;
    int64_t remaining_balance;
    int start_day;
    int next_emi_day;
    int missed_emis;
    LoanStatus status;
    char approved_by[16];
    char second_approved_by[16];
} Loan;

/* Voucher */
typedef struct {
    uint64_t id;
    uint8_t code_hash[HASH_SIZE];
    VoucherType type;
    int64_t value;
    int expiry_day;
    int max_total_uses;
    int total_uses;
    int uses_per_account;
    VoucherStatus status;
    char created_by[16];
} Voucher;

/* Standing Order */
typedef struct {
    uint64_t id;
    long account_id;
    long dest_account_id;
    int64_t amount;
    Frequency frequency;
    int start_day;
    int end_day;
    int remaining_count;
    int last_run_day;
    int consecutive_failures;
    bool is_active;
} StandingOrder;

/* Security Alert (F2) */
typedef struct {
    uint64_t id;
    int day;
    long account_id;
    int rule_id;
    int risk_score;
    AlertStatus status;
    char message[80];
} Alert;

/* Audit Log Entry (F7) */
typedef struct {
    uint64_t seq;
    time_t clock;
    char actor[16];
    char action[24];
    char target[16];
    char detail[64];
    uint8_t prev_hash[HASH_SIZE];
    uint8_t hash[HASH_SIZE];
} AuditEntry;

/* Account Budget Configuration (F10) */
typedef struct {
    long account_id;
    TxnType type;
    int64_t monthly_limit;
    bool is_active;
} AccountBudget;

/* Job Ledger for Idempotent Scheduler (F9) */
typedef struct {
    int last_snapshot_day;
    int last_standing_day;
    int last_loan_day;
    int last_interest_day;
    int last_voucher_day;
    int last_reset_day;
    int last_fraud_day;
    int last_day_end;
} JobLedger;

/* Bank System Root State */
typedef struct {
    Config config;
    int sim_day;
    
    Account accounts[MAX_ACCOUNTS];
    size_t account_count;
    
    Txn txns[MAX_TXNS_TOTAL];
    size_t txn_count;
    
    Admin admins[MAX_ADMINS];
    size_t admin_count;
    
    Loan loans[MAX_LOANS];
    size_t loan_count;
    
    Voucher vouchers[MAX_VOUCHERS];
    size_t voucher_count;
    
    StandingOrder standing_orders[MAX_STANDING_ORDERS];
    size_t standing_order_count;
    
    Alert alerts[MAX_ALERTS];
    size_t alert_count;

    AccountBudget budgets[MAX_ACCOUNTS];
    size_t budget_count;
    
    uint8_t audit_head_hash[HASH_SIZE];
    uint64_t next_txn_id;
    uint64_t next_loan_id;
    uint64_t next_voucher_id;
    uint64_t next_so_id;
    uint64_t next_alert_id;
    uint64_t next_audit_seq;

    JobLedger ledger;
    int64_t balance_snapshots[MAX_ACCOUNTS][30];
    int snapshot_count[MAX_ACCOUNTS];
} Bank;

#endif /* TYPES_H */
