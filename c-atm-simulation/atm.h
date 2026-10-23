/**
 * @file atm.h
 * @brief Header file for Console-based ATM Simulation in C (C11 Standard)
 * Complete implementation of Core Features and Advanced Features A1 to A22.
 */

#ifndef ATM_H
#define ATM_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>
#include <stdint.h>
#include <ctype.h>

/* ========================================================================== */
/*                           SYSTEM CONSTANTS & LIMITS                        */
/* ========================================================================== */

#define ATM_MAGIC_HEADER       0x41544D31U /* "ATM1" in ASCII hex */
#define ATM_VERSION            1U
#define DEFAULT_ADMIN_PIN      "9999"
#define MAX_NAME_LEN           64
#define MAX_PIN_LEN            8
#define MAX_SALT_LEN           17
#define MAX_HASH_LEN           65
#define MAX_REF_LEN            32
#define MAX_DESC_LEN           128
#define MAX_NOTE_LEN           128
#define MAX_REASON_LEN         128
#define MAX_TXNS_PER_ACC       100
#define MAX_FDS_PER_ACC        10
#define MAX_BENEFICIARIES      10
#define MAX_RECURRING_BILLS    10
#define MAX_NOTIFICATIONS      20
#define MAX_DISPUTES_GLOBAL    100

#define ACCOUNTS_FILE          "accounts.dat"
#define ACCOUNTS_BACKUP_FILE   "accounts.bak"
#define ACCOUNTS_TEMP_FILE     "accounts.tmp"
#define AUDIT_LOG_FILE         "audit.log"
#define CONFIG_FILE            "bank_config.txt"
#define RATES_FILE             "rates.txt"
#define STATEMENT_CSV_FILE     "statement.csv"

/* ========================================================================== */
/*                             ENUMERATIONS                                   */
/* ========================================================================== */

/**
 * @brief Account types supporting differential policies (A3)
 */
typedef enum {
    ACCOUNT_SAVINGS = 0,
    ACCOUNT_CURRENT = 1,
    ACCOUNT_STUDENT = 2
} AccountType;

/**
 * @brief Account status states for security and fraud containment (Core, A2, A21)
 */
typedef enum {
    STATUS_ACTIVE = 0,
    STATUS_BLOCKED = 1,      /* Locked due to 3 invalid PIN attempts */
    STATUS_FROZEN_FRAUD = 2  /* Locked due to automated fraud triggers (A2) */
} AccountStatus;

/**
 * @brief Categories of financial transactions (Core, A4, A5, A12, A13)
 */
typedef enum {
    TXN_DEPOSIT = 0,
    TXN_WITHDRAWAL = 1,
    TXN_TRANSFER_OUT = 2,
    TXN_TRANSFER_IN = 3,
    TXN_BILL_PAYMENT = 4,
    TXN_REVERSAL = 5,
    TXN_INTEREST_CREDIT = 6,
    TXN_FEE_CHARGE = 7,
    TXN_FD_CREATE = 8
} TransactionType;

/**
 * @brief Utility and utility billing categories (Core, A16)
 */
typedef enum {
    BILL_ELECTRICITY = 0,
    BILL_GAS = 1,
    BILL_INTERNET = 2,
    BILL_MOBILE = 3
} BillCategory;

/**
 * @brief Dispute tracking lifecycle states (A20)
 */
typedef enum {
    DISPUTE_OPEN = 0,
    DISPUTE_UNDER_REVIEW = 1,
    DISPUTE_RESOLVED = 2,
    DISPUTE_REJECTED = 3
} DisputeStatus;

/**
 * @brief Physical and logical status of the ATM terminal (A22)
 */
typedef enum {
    ATM_OPERATIONAL = 0,
    ATM_MAINTENANCE = 1
} AtmStatus;

/**
 * @brief Sort orders for statement tools using qsort (A9)
 */
typedef enum {
    SORT_NEWEST_FIRST = 1,
    SORT_OLDEST_FIRST = 2,
    SORT_HIGHEST_AMOUNT = 3
} StatementSortOrder;

/* ========================================================================== */
/*                             DATA STRUCTURES                                */
/* ========================================================================== */

/**
 * @brief Transaction record holding audit, ledger, and reversal data (Core, A12, A13)
 */
typedef struct {
    char ref_id[MAX_REF_LEN];              /* Unique transaction reference (A12) */
    TransactionType type;                  /* Type of transaction */
    double amount;                         /* Nominal transaction sum */
    double fee;                            /* Assessed fee (A4) */
    double balance_after;                  /* Resulting ledger balance */
    time_t timestamp;                      /* UNIX epoch timestamp */
    int related_account;                   /* Counterparty or biller identifier */
    char description[MAX_DESC_LEN];        /* Human-readable annotation */
    bool is_reversed;                      /* True if reversed by administrator (A13) */
    char reversal_ref[MAX_REF_LEN];        /* Reference ID of reversing compensating transaction */
} Transaction;

/**
 * @brief Fixed Deposit investment record (A5)
 */
typedef struct {
    int fd_id;                             /* Unique FD identifier */
    double principal;                      /* Principal sum locked */
    double interest_rate;                  /* Guaranteed annual interest rate (%) */
    int duration_months;                   /* Term length in months */
    time_t start_date;                     /* Creation timestamp */
    time_t maturity_date;                  /* Maturity epoch timestamp */
    double maturity_amount;                /* Projected maturity payout */
    bool is_active;                        /* Active status */
} FixedDeposit;

/**
 * @brief Saved beneficiary for accelerated fund transfers (A11)
 */
typedef struct {
    int account_number;                    /* Validated destination account number */
    char name[MAX_NAME_LEN];               /* Confirmed recipient legal name */
    char nickname[32];                     /* Friendly alias */
} Beneficiary;

/**
 * @brief Recurring bill payment instruction (A16)
 */
typedef struct {
    int instruction_id;                    /* Instruction ID */
    BillCategory category;                 /* Bill service type */
    char biller_name[32];                  /* Service provider name */
    char account_or_meter[32];             /* Customer reference / meter ID */
    double amount;                         /* Fixed recurring sum */
    time_t next_due_date;                  /* Next scheduled execution epoch */
    bool is_active;                        /* Paused / Active flag */
} RecurringPayment;

/**
 * @brief Customer notification record (A14)
 */
typedef struct {
    int notification_id;                   /* Notification ID */
    time_t timestamp;                      /* Generated timestamp */
    char message[MAX_NOTE_LEN];            /* Notification payload */
    bool is_read;                          /* Read indicator */
} Notification;

/**
 * @brief Customer transaction dispute case (A20)
 */
typedef struct {
    int dispute_id;                        /* Unique dispute serial */
    char txn_ref[MAX_REF_LEN];             /* Disputed transaction reference */
    int account_number;                    /* Complaining customer account */
    char reason[MAX_REASON_LEN];           /* Grounds for dispute */
    time_t filed_at;                       /* Filing timestamp */
    DisputeStatus status;                  /* Lifecycle status */
    char resolution_note[MAX_NOTE_LEN];    /* Admin resolution rationale */
    int resolved_by_admin;                 /* Admin identifier */
    time_t resolved_at;                    /* Resolution timestamp */
} Dispute;

/**
 * @brief Core Account entity holding identity, credentials, balance, and child records
 */
typedef struct {
    int account_number;                    /* Unique positive account number */
    char name[MAX_NAME_LEN];               /* Account holder legal name */
    char pin_salt[MAX_SALT_LEN];           /* Hexadecimal cryptographic salt (A21) */
    char pin_hash[MAX_HASH_LEN];           /* Salted cryptographic PIN hash (Core, A21) */
    bool must_change_pin;                  /* Flag forcing PIN change on login (A21) */
    AccountType type;                      /* Savings, Current, or Student (A3) */
    AccountStatus status;                  /* Active, Blocked, or Frozen (Core, A2) */
    double balance;                        /* Cleared ledger balance */
    double low_balance_threshold;          /* Custom threshold for alerts (A15) */
    int failed_pin_attempts;               /* Consecutive bad PIN count */
    int failed_otp_attempts;               /* Consecutive bad OTP count (A2) */
    double daily_withdrawn;                /* Amount withdrawn today */
    double daily_transferred;              /* Amount transferred today */
    time_t last_daily_reset;               /* Timestamp of last daily window reset */
    int monthly_withdrawals_count;         /* Count of withdrawals in current calendar month (A4) */
    
    /* Fraud velocity detection window (A2: 3 withdrawals within 120s) */
    time_t withdrawal_timestamps[3];
    int withdrawal_ts_count;

    /* Embedded Transaction History */
    Transaction transactions[MAX_TXNS_PER_ACC];
    int txn_count;

    /* Embedded Fixed Deposits (A5) */
    FixedDeposit fixed_deposits[MAX_FDS_PER_ACC];
    int fd_count;

    /* Embedded Beneficiaries (A11) */
    Beneficiary beneficiaries[MAX_BENEFICIARIES];
    int beneficiary_count;

    /* Embedded Recurring Payments (A16) */
    RecurringPayment recurring_payments[MAX_RECURRING_BILLS];
    int recurring_count;

    /* Embedded Notifications (A14) */
    Notification notifications[MAX_NOTIFICATIONS];
    int notification_count;
} Account;

/**
 * @brief ATM physical cash dispenser notes inventory (A10, A22)
 */
typedef struct {
    int notes_1000;                        /* Count of 1,000 denomination notes */
    int notes_500;                         /* Count of 500 denomination notes */
    int notes_100;                         /* Count of 100 denomination notes */
} CashInventory;

/**
 * @brief Dynamic configuration parameters loaded from bank_config.txt (A19)
 */
typedef struct {
    int session_timeout_sec;               /* Inactivity timeout in seconds (A8) */
    int max_pin_attempts;                  /* Card block threshold (Core) */
    double otp_threshold;                  /* Transfer sum triggering OTP (A1) */
    int otp_expiry_sec;                    /* OTP validity window (A1) */
    double savings_min_balance;            /* Minimum balance for Savings (A3) */
    double current_min_balance;            /* Minimum balance for Current (A3) */
    double student_min_balance;            /* Minimum balance for Student (A3) */
    double savings_daily_limit;            /* Daily withdrawal limit: Savings */
    double current_daily_limit;            /* Daily withdrawal limit: Current */
    double student_daily_limit;            /* Daily withdrawal limit: Student */
    double transfer_fee_percent;           /* Fee percentage for transfers (A4) */
    int free_withdrawals_per_month;        /* Threshold before withdrawal surcharge (A4) */
    double extra_withdrawal_fee;           /* Surcharge per extra withdrawal (A4) */
    double savings_annual_interest;        /* Annual interest rate: Savings (%) */
    double current_annual_interest;        /* Annual interest rate: Current (%) */
    double student_annual_interest;        /* Annual interest rate: Student (%) */
    int fraud_velocity_count;              /* Count of withdrawals triggering freeze */
    int fraud_velocity_seconds;            /* Window for withdrawal velocity check */
    double fraud_transfer_pct;             /* Fraction of balance triggering freeze */
    int low_notes_threshold;               /* Cassette note count for low-stock warning */
    char admin_pin_salt[MAX_SALT_LEN];     /* Administrator PIN salt */
    char admin_pin_hash[MAX_HASH_LEN];     /* Administrator salted PIN hash */
} BankConfig;

/**
 * @brief Foreign exchange rates relative to base currency (BDT) (A6)
 */
typedef struct {
    double bdt_per_usd;                    /* Rate: 1 USD in BDT */
    double bdt_per_eur;                    /* Rate: 1 EUR in BDT */
} CurrencyRates;

/**
 * @brief Binary persistence file header with integrity checksum (A18)
 */
typedef struct {
    uint32_t magic;                        /* Magic bytes: 0x41544D31 */
    uint32_t version;                      /* Format version: 1 */
    uint32_t account_count;                /* Total serialized accounts */
    uint32_t dispute_count;                /* Total serialized disputes */
    uint32_t checksum;                     /* Adler-32 / CRC32 integrity checksum */
    int notes_1000;                        /* Hardware inventory snapshot */
    int notes_500;
    int notes_100;
    int atm_status;                        /* Operational / Maintenance mode */
    time_t saved_at;                       /* Serialization timestamp */
} FileHeader;

/**
 * @brief Global ATM terminal operating runtime context
 */
typedef struct {
    AtmStatus operational_status;          /* ATM operational mode (A22) */
    CashInventory cash;                    /* Note inventory (A10) */
    BankConfig config;                     /* Loaded configuration (A19) */
    CurrencyRates rates;                   /* Currency rates (A6) */
    Dispute disputes[MAX_DISPUTES_GLOBAL]; /* Global transaction disputes (A20) */
    int dispute_count;
    time_t last_activity_time;             /* Timestamp for session timeout tracking (A8) */
} AtmContext;

/* ========================================================================== */
/*                            FUNCTION PROTOTYPES                             */
/* ========================================================================== */

/* --- Security & Cryptography (Core, A21) --- */
void generate_salt(char *salt, size_t length);
void compute_pin_hash(const char *pin, const char *salt, char *output_hash);
bool verify_pin(const char *input_pin, const char *salt, const char *stored_hash);
void generate_reference_id(char *buffer, size_t size);

/* --- Configuration & Currency Rates (A6, A19) --- */
void load_default_config(BankConfig *config);
bool load_config_file(const char *filename, BankConfig *config);
bool save_config_file(const char *filename, const BankConfig *config);
void load_default_rates(CurrencyRates *rates);
bool load_rates_file(const char *filename, CurrencyRates *rates);
bool save_rates_file(const char *filename, const CurrencyRates *rates);

/* --- Audit Logging (A7) --- */
void audit_log(const char *action, int account_num, const char *details);

/* --- Data Integrity, Persistence & Recovery (A18) --- */
uint32_t calculate_checksum(const void *data, size_t bytes);
bool save_all_data(const char *filename);
bool load_all_data(const char *filename);
bool recover_from_backup(void);
void init_demo_accounts(void);
void cleanup_system_memory(void);

/* --- Input Validation & Console Helpers --- */
void clear_screen(void);
void press_enter_to_continue(void);
bool get_line_input(char *buffer, size_t size);
bool get_int_input(const char *prompt, int *out_val, int min_val, int max_val);
bool get_double_input(const char *prompt, double *out_val, double min_val, double max_val);
void print_header(const char *title);
void print_receipt(const Transaction *txn, const Account *acc);

/* --- Account Query & Management --- */
Account *find_account(int account_number);
Account *create_account(int account_number, const char *name, const char *pin, AccountType type, double initial_deposit);
bool delete_account(int account_number);
double get_min_balance_for_type(AccountType type, const BankConfig *config);
double get_daily_limit_for_type(AccountType type, const BankConfig *config);
void check_and_reset_daily_limits(Account *acc);

/* --- Transactions Engine (Core, A4, A10, A12, A13) --- */
bool record_transaction(Account *acc, TransactionType type, double amount, double fee, int related_acc, const char *desc, char *out_ref);
bool perform_deposit(Account *acc, double amount);
bool perform_withdrawal(Account *acc, double amount, AtmContext *atm);
bool perform_transfer(Account *sender, Account *recipient, double amount, AtmContext *atm);
bool perform_bill_payment(Account *acc, BillCategory cat, const char *biller, const char *ref_num, double amount);
bool perform_reversal(const char *target_ref_id, const char *reason, int admin_id, AtmContext *atm);

/* --- Cash Inventory & Dispenser Logic (A10, A22) --- */
bool dispense_cash(double amount, CashInventory *inventory, int *out_1000, int *out_500, int *out_100);
void refill_cash(CashInventory *inventory, int add_1000, int add_500, int add_100);
int get_total_cash_value(const CashInventory *inventory);

/* --- Advanced Features Implementations --- */
/* A1: OTP Verification */
bool verify_otp_flow(Account *acc, double amount, const BankConfig *config);

/* A2: Fraud Velocity & Risk Checks */
bool check_and_apply_fraud_rules(Account *acc, double amount, bool is_transfer, const BankConfig *config);

/* A4: End of Month Interest & Fees */
void process_end_of_month(AtmContext *atm);

/* A5: Fixed Deposit & Loan EMI */
bool open_fixed_deposit(Account *acc, double principal, int months, double annual_rate);
void calculate_loan_emi_schedule(double principal, double annual_rate, int months);

/* A6: Multi-Currency Converter */
void display_multi_currency_balances(const Account *acc, const CurrencyRates *rates);

/* A8: Session Timeout */
bool check_session_timeout(AtmContext *atm);
void touch_session(AtmContext *atm);

/* A9: Statement Search, Sort, Export */
void search_and_export_statements(Account *acc);

/* A11: Beneficiary Management */
void manage_beneficiaries(Account *acc);

/* A14, A15: Notifications & Account Health */
void add_notification(Account *acc, const char *message);
void view_notifications(Account *acc);
void display_account_health(const Account *acc, const BankConfig *config);

/* A16: Recurring Payments */
void manage_recurring_payments(Account *acc);
void process_scheduled_recurring_bills(AtmContext *atm);

/* A17: Reports & Analytics */
void generate_customer_analytics(const Account *acc, const BankConfig *config);
void generate_admin_financial_report(const AtmContext *atm);

/* A20: Customer Disputes */
void file_customer_dispute(Account *acc, AtmContext *atm);
void manage_admin_disputes(AtmContext *atm);

/* A21: PIN Change & Reset */
bool customer_change_pin(Account *acc);
bool admin_reset_customer_pin(Account *acc);

/* --- Menu Interfaces --- */
void customer_menu(Account *acc, AtmContext *atm);
void admin_menu(AtmContext *atm);
void main_atm_loop(AtmContext *atm);

#endif /* ATM_H */
