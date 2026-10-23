# Rahat's C Projects Repository

A collection of C programming projects, algorithmic implementations, and system simulations.

---

## Projects

### C ATM Simulation
A modular, production-grade console ATM and banking system simulation in ISO C11 with zero external libraries.

- **F1 (Simulated OTP):** 6-digit cryptographic OTPs with TTL expiry, 3-strike lockout, and resend limits delivered via simulated SMS.
- **F2 (Fraud Alert Engine):** Continuous 0–100 risk scoring with automated notices, step-up OTPs, and card freezing across 6 rules.
- **F3 (Admin Accounts):** Role-based admin portal (SUPERADMIN, ADMIN, AUDITOR) with forced initial password changes and audit tracking.
- **F4 (Saved Data):** Little-endian binary serialization with CRC-32 integrity validation, atomic file rotations, and single-writer locking.
- **F5 (Loans):** Multi-tier loan application and amortization schedules with dual approvals and whole-cent rounding absorption.
- **F6 (Vouchers):** Crockford-base32 alphanumeric promotional vouchers with check digits and SHA-256 hashed storage.
- **F7 (Crypto & Audit):** Handcrafted SHA-256, HMAC-SHA256, CRC-32, PBKDF-style PIN hashing, and tamper-proof hash chains.
- **F8 (Client/Server Sockets):** Single-threaded event-driven bank daemon (`bankd`) and ATM client (`atm`) with challenge-response auth over ATMP/1.
- **F9 (Scheduler):** Simulated daily clock driving automated loan EMI debits, standing orders, daily interest accrual, and fraud risk decay.
- **F10 (Statements & Analytics):** Double-entry balance reconciliation, RFC 4180 CSV exports, 60-column ASCII spending analytics, and category budgets.

- **Tech:** ISO C11, standard library only, OS sockets (Winsock2 / POSIX) for client/server networking.
- **Build & Run:**
  ```bash
  cd c-atm-simulation
  make all && make test
  ./atm --standalone       # Standalone in-process terminal
  ./bankd                  # Run central bank daemon
  ./atm                    # Run ATM client connected to daemon
  ```
- **Demo Accounts (Demo Only):**
  - Account `1001`, Demo PIN: `1234` (Demo Only)
  - Account `1002`, Demo PIN: `2345` (Demo Only)
  - Account `1003`, Demo PIN: `3456` (Demo Only)
  - Admin `ADMIN001`, Demo Passphrase: `admin` (Demo Only)
- **Folder Link:** [c-atm-simulation](./c-atm-simulation)

*Educational project: no real money or banking data*
