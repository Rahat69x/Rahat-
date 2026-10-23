# C ATM Simulation (Production-Grade Terminal Bank in ISO C11)

> **EDUCATIONAL PROJECT:** No real money, financial accounts, or live cardholder data are managed. All SMS verification codes and outbound notices are simulated on-screen (`[SIMULATED SMS]`).

---

## 1. Overview

**C ATM Simulation** is a robust, modular, client-server banking system written strictly in **ISO C11** using **only the C standard library** and OS-level socket APIs (POSIX BSD sockets and Windows Winsock2).

It models core commercial banking infrastructure: challenge-response cryptographic authentication, rule-based fraud detection, multi-tier loan amortization, voucher generation with check digits, recurring scheduled jobs with standing orders and daily interest accrual, double-entry transaction ledgers, RFC 4180 statement exports, and tamper-proof audit trails.

---

## 2. Implemented Features (F1 – F10)

| ID | Feature | Description |
|---|---|---|
| **F1** | **Simulated OTP Codes** | 6-digit cryptographic OTPs with TTL expiry, 3-strike lockout, and resend rate limits; delivered via on-screen simulated SMS. |
| **F2** | **Fraud Alert Rule Engine** | Continuous risk scoring (0–100) evaluating velocity, large amounts, new payees, odd hours, auth failures, and account drains; triggers warnings, step-up OTPs, and automated card freezes. |
| **F3** | **Role-Based Admin Accounts** | Multi-tier admin access (`SUPERADMIN`, `ADMIN`, `AUDITOR`) with forced first-login passphrase change, account unfreezing, card limit overrides, and customer PIN resets. |
| **F4** | **Fault-Tolerant Persistence** | Field-by-field little-endian binary storage (`bank.dat`, `txns.dat`) with CRC-32 checksums, atomic `.tmp` -> `.bak` -> `.dat` rotation, and exclusive process locks. |
| **F5** | **Loans & Amortization** | Personal and short-term loans with eligibility gating, dual-admin approvals above thresholds, whole-cent amortization scheduling, and rounding absorption. |
| **F6** | **Promotional Vouchers** | 12-character Crockford-base32 vouchers with check-digit validation, one-time hashes, per-account quotas, and brute-force protection. |
| **F7** | **Cryptography & Tamper-Proof Audit** | Handcrafted SHA-256 (FIPS 180-4), HMAC-SHA256, CRC-32, PBKDF-style iterated PIN hashing, and cryptographic append-only audit hash chain. |
| **F8** | **Client-Server Architecture** | Single-threaded event-driven bank daemon (`bankd`) and terminal client (`atm`) communicating over ATMP/1 with zero-knowledge challenge-response authentication. |
| **F9** | **Simulated Clock & Scheduler** | Discrete simulated daily ticks executing standing orders, loan EMI debits, late fees, daily compound interest with fractional carry, and risk decay. |
| **F10** | **Statements & ASCII Analytics** | Double-entry reconciliation verification, RFC 4180 CSV / formatted TXT statement exports, 60-column ASCII spending analytics, and category budgets. |

---

## 3. Strict Architectural Constraints (C1 – C8)

- **C1 (Language):** ISO C11 only. Compiles cleanly under GCC/Clang with `-std=c11 -Wall -Wextra -Wpedantic -Werror` with zero warnings.
- **C2 (Zero External Dependencies):** Standard C library only. Handcrafted SHA-256, HMAC, CRC-32, INI parser, and serialization. Sockets (`<sys/socket.h>` / Winsock2) are the sole OS-level exception.
- **C3 (Safe I/O):** Bounded `fgets`-based input parsing (`input.c`) and safe formatted writes. No `gets`, `scanf("%s")`, `strcpy`, or buffer overflows.
- **C4 (Exact Minor-Unit Currency):** All balances and monetary figures are tracked as 64-bit signed integers (`int64_t`) in minor currency units (cents: $1.00 = 100). No floating-point roundoff errors.
- **C5 (Zero Secret Leakage):** Customer PINs, OTP codes, admin passphrases, and voucher secrets are never stored or logged in plain text. Account numbers are masked (`****1234`).
- **C6 (Injectable Clock):** Time is injected via `clock_now()` and `sim_day()` for deterministic testability.
- **C7 (Memory Safety):** Clean memory management and clean test execution across all suites.
- **C8 (Modular Design):** Clean separation of concerns with encapsulated modules, functions strictly under ~60 lines, and a transport-agnostic `BankApi` interface.

---

## 4. Built-in Demo Accounts (DEMO ONLY)

> [!WARNING]
> These credentials are provided exclusively for testing and local simulation.

| Account ID | Holder Name | Demo PIN (DEMO ONLY) | Starting Balance |
|---|---|---|---|
| `1001` | Alice Smith | `1234` | $50,000.00 |
| `1002` | Bob Jones | `2345` | $75,000.00 |
| `1003` | Charlie Day | `3456` | $25,000.00 |

**Default Admin Account (DEMO ONLY):**
- ID: `ADMIN001`
- Temporary Passphrase: `admin` (forced to change at initial login)

---

## 5. Build & Test Instructions

### 5.1 Prerequisites
- GCC / MinGW-w64 with C11 support.
- `make` or `mingw32-make`.

### 5.2 Build All Targets
```bash
# Build atm.exe and bankd.exe
make all
```

### 5.3 Run Full Automated Test Suite
```bash
# Compiles and runs all 12 test suites (110 automated test cases)
make test
```

### 5.4 Run in Standalone Mode
Run the ATM terminal directly in-process without launching the network server:
```bash
./atm --standalone
```

### 5.5 Run in Client-Server Network Mode
1. **Start the Bank Daemon (`bankd`):**
   ```bash
   ./bankd
   ```
2. **In a separate terminal, launch the ATM Client (`atm`):**
   ```bash
   ./atm
   ```

### 5.6 Verify Cryptographic Audit Chain
Traverse and verify the SHA-256 hash chain of the audit log against the database head hash:
```bash
./atm --verify-audit
```

---

## 6. Project Layout

```
c-atm-simulation/
├── Makefile                 # Strict C11 build rules and test targets
├── README.md                # Project documentation
├── config.ini               # Hand-parsed runtime configuration
├── legacy/atm_basic.c       # Preserved baseline implementation
├── include/                 # Public C headers (one per module)
├── src/
│   ├── core/                # bank.c, account.c, txn.c, money.c, clock.c
│   ├── security/            # sha256.c, hmac.c, crc32.c, rng.c, pin.c, audit.c, otp.c, fraud.c
│   ├── storage/             # config.c, store.c
│   ├── features/            # admin.c, loans.c, vouchers.c, standing.c, scheduler.c, statements.c, analytics.c
│   ├── net/                 # net_compat.h, proto.c, server.c, client.c
│   ├── ui/                  # input.c, render.c, menu_customer.c, menu_admin.c
│   ├── main_atm.c           # Terminal ATM client entrypoint
│   └── main_bankd.c         # Central bank server daemon entrypoint
├── tests/
│   ├── unit/                # Unit test suites (F1-F7, F9, F10)
│   ├── regression/          # Baseline regression test suite
│   └── integration/         # F8 socket stress and concurrency test suite
├── docs/                    # protocol.md, file-format.md, security-notes.md
└── data/                    # Runtime database files (git-ignored)
```
