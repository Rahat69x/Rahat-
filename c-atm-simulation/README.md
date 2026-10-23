# Console-Based ATM Simulation in C (C11 Standard)

A modular, robust, and pedagogical implementation of an Automated Teller Machine (ATM) simulation written in C conforming strictly to the ISO C11 standard (`-std=c11`).

> **Educational Disclaimer:** This software is designed strictly as an educational console simulation for computer science students and engineers. It is **not** production banking software and lacks hardware security module (HSM) key isolation, multi-party transaction consensus, ISO 8583 financial network interfaces, and audited multi-factor authentication.

---

## 1. System Architecture & Features

The system implements the **Core Banking Specification** and all **Advanced Modules A1 through A22**:

| Feature ID | Module Name | Implementation Status | Description |
|:---|:---|:---|:---|
| **Core 1** | Secure Customer Login | **Fully Implemented** | Account number + 4-digit PIN verified via salted iterative cryptographic hash. Lockout after 3 invalid attempts. |
| **Core 2** | Customer Banking Portal | **Fully Implemented** | Balance inquiry, cash deposit, cash withdrawal, fund transfer, bill payments, mini-statements, PIN changes, receipts. |
| **Core 3** | Administrator Control Console | **Fully Implemented** | Dedicated admin PIN (`9999`), account provisioning, card unblocking/blocking, account deletion, and PIN reset. |
| **Core 4** | Data Persistence & Demo Engine | **Fully Implemented** | Atomic persistence (`accounts.tmp` -> `accounts.dat`), verified backup (`accounts.bak`), and automatic 3 demo accounts on bootstrap. |
| **A1** | One-Time Passcode (OTP) | **Fully Implemented** | Simulated SMS OTP for transfers $\ge$ 10,000 BDT, expiring after 60 seconds with 3 allowed attempts. |
| **A2** | Automated Fraud Detection | **Fully Implemented** | Velocity freeze (3 withdrawals in 120s), high-value transfer anomaly (> 80% balance), and 2 consecutive failed OTPs. |
| **A3** | Differential Account Types | **Fully Implemented** | Savings, Current, and Student tiers with custom minimum balances, daily limits, interest rates, and fee rules. |
| **A4** | Fees & Interest Engine | **Fully Implemented** | 0.5% transfer fee, monthly surcharge on withdrawals exceeding free quota, and monthly savings interest accrual. |
| **A5** | Fixed Deposit & Loan Calculator | **Fully Implemented** | Term FD booking with maturity yields and full loan amortization schedule generator with monthly EMI breakdown. |
| **A6** | Multi-Currency Conversion | **Fully Implemented** | Real-time conversion of BDT ledger balances into USD and EUR based on configuration table `rates.txt`. |
| **A7** | Immutable Audit Trail | **Fully Implemented** | Append-only logging of all events, transactions, security exceptions, and administrative interventions to `audit.log`. |
| **A8** | Inactivity Session Timeout | **Fully Implemented** | Automatic customer session termination after 60 seconds of inactivity using `time()` checks. |
| **A9** | Statement Analytics & CSV Export| **Fully Implemented** | Multi-parameter filtering, `qsort()` ordering (newest, oldest, highest value), and export to `statement.csv`. |
| **A10** | Physical Cash Dispenser Stock | **Fully Implemented** | Real-time cassette inventory (1000, 500, 100 BDT notes), greedy note dispenser algorithm, and low-stock alerts. |
| **A11** | Beneficiary Management | **Fully Implemented** | Add, review, remove, and transfer to validated beneficiary accounts with duplicate prevention. |
| **A12** | Unique Transaction Reference | **Fully Implemented** | High-entropy formatted references (`TXN-YYYYMMDD-HHMMSS-XXXX`) guaranteeing idempotency and duplicate prevention. |
| **A13** | Admin Transaction Reversal | **Fully Implemented** | Reversal of deposits, withdrawals, and transfers with compensating ledger records and balance solvency checks. |
| **A14** | Customer Notifications | **Fully Implemented** | Persistent notification inbox recording security alerts, monthly interest credits, and account status transitions. |
| **A15** | Low-Balance Alerts & Health | **Fully Implemented** | Customizable balance alert threshold and comprehensive liquidity health breakdown (ledger, reserved, available). |
| **A16** | Recurring Scheduled Payments | **Fully Implemented** | Recurring bill instructions for utility and telecom services with simulated execution cycles. |
| **A17** | Financial Reports & Analytics | **Fully Implemented** | Detailed customer cashflow analytics and macro administrative portfolio liability reports. |
| **A18** | High-Integrity Recovery Engine | **Fully Implemented** | Binary file format header (Magic `ATM1`, Version 1), Adler-32 payload checksum verification, safe write, and backup recovery. |
| **A19** | Dynamic Rules Configuration | **Fully Implemented** | Key-value configuration engine (`bank_config.txt`) controlling limits, fees, timeouts, and fraud rules. |
| **A20** | Customer Transaction Disputes | **Fully Implemented** | Customer dispute ticket submission, administrative review lifecycle (`OPEN`, `UNDER_REVIEW`, `RESOLVED`, `REJECTED`). |
| **A21** | Salted PIN Cryptography | **Fully Implemented** | Per-account cryptographic salt, 5,000-round FNV-1a stretching, and mandatory temporary PIN change enforcement. |
| **A22** | ATM Terminal Maintenance Mode | **Fully Implemented** | Administrative maintenance mode toggle disabling cash withdrawals while keeping non-cash operations accessible. |

---

## 2. Project File Structure

```
atm-simulation/
├── atm.h              # Unified header containing structs, enums, macros, and prototypes
├── atm.c              # Core and advanced operational business logic and UI loops
├── main.c             # Application entry point, configuration bootstrapping, and shutdown
├── bank_config.txt    # Editable banking parameters, fees, limits, and security thresholds
├── rates.txt          # Foreign exchange conversion rates (USD/BDT, EUR/BDT)
├── .gitignore         # Build and runtime data ignore rules
└── README.md          # Comprehensive documentation and technical specifications
```

---

## 3. Compilation & Execution

### Prerequisites
- GCC Compiler with C11 support (MinGW-w64 on Windows, GCC on Linux, Apple Clang on macOS).

### Compilation Command
Compile cleanly with all warnings enabled:
```bash
gcc -Wall -Wextra -std=c11 main.c atm.c -o atm.exe
```
*(On Linux/macOS, use `-o atm` instead of `-o atm.exe`)*

### Execution
```bash
./atm.exe
```

---

## 4. Default Credentials & Demo Accounts

The system automatically initializes 3 demo accounts upon first launch if `accounts.dat` is not present:

| Account Number | Legal Name | PIN | Account Type | Initial Balance |
|:---|:---|:---|:---|:---|
| **1001** | Alice Rahman | `1234` | Savings | 25,000.00 BDT |
| **1002** | Bob Chowdhury | `4321` | Current | 85,000.00 BDT |
| **1003** | Charlie Hasan | `1111` | Student | 5,000.00 BDT |

### Administrator Credentials
- **Access Option:** Select Option `2` from the main menu.
- **Default Admin PIN:** `9999`

---

## 5. Storage & File Formats

| File Name | Format | Purpose |
|:---|:---|:---|
| `accounts.dat` | Binary (`ATM1` magic, Version 1) | Primary database with Adler-32 payload checksum. |
| `accounts.bak` | Binary | Verified rollback backup copy. |
| `accounts.tmp` | Binary | Atomic write buffer before replacing primary database. |
| `audit.log` | Formatted Text (Append-only) | Immutable operational and security audit log. |
| `bank_config.txt` | Key-Value Text | Dynamic banking parameters, limits, fees, and thresholds. |
| `rates.txt` | Key-Value Text | Foreign exchange rates relative to BDT base. |
| `statement.csv` | CSV | Exported account transaction history. |

---

## 6. Short Sample Run

```
================================================================================
           ANTIGRAVITY CONSOLE ATM SIMULATION (C11 STANDARD)                    
================================================================================
[*] Verifying database integrity...
[+] Verified: Primary database loaded with integrity validation (A18).

================================================================================
                           CUSTOMER SECURE LOGIN          
================================================================================
Enter your 4-digit Account Number (e.g. 1001): 1001
Enter your 4-digit Secret PIN: 1234
[+] Authentication Successful. Welcome, Alice Rahman!

Select an option [0-14]: 2
Enter withdrawal amount (multiples of 100): BDT 2000
+-------------------------------------------------------------+
|                       OFFICIAL ATM RECEIPT                  |
+-------------------------------------------------------------+
| Reference ID : TXN-20261023-035045-1002                     |
| Customer Name: Alice Rahman                                 |
| Amount       : BDT 2000.00                                  |
| New Balance  : BDT 23000.00                                 |
+-------------------------------------------------------------+
```

