# Security Architecture & Limitations

## 1. Educational Scope & Disclaimer

> [!CAUTION]
> **SIMULATION & EDUCATIONAL USE ONLY**
> This project is designed strictly for educational and pedagogical purposes. No real money, financial accounts, or live cardholder data are managed. All SMS verification codes and outbound notices are simulated on-screen (`[SIMULATED SMS]`).

---

## 2. PIN Cryptography & Honest Security Boundaries

### 2.1 The Reality of 4-Digit PINs
- A 4-digit numeric PIN has exactly **10,000 possible combinations** ($10^4$).
- Even with 10,000 rounds of PBKDF-style iterated SHA-256, an offline attacker with access to a hashed PIN can brute-force all 10,000 candidates in a fraction of a second on modern hardware.
- **The primary defense is the lockout rule:** The account is permanently locked after `max_pin_tries` (3) consecutive failed attempts.
- Our custom PBKDF implementation demonstrates the concept of iterative key stretching with salts, but is **not** a replacement for modern memory-hard password hashing algorithms such as Argon2id or bcrypt.

### 2.2 Constant-Time Comparisons
Standard `memcmp` or `strcmp` terminates on the first differing byte, leaking timing information through side-channels. PIN verification and challenge HMAC comparisons use `pin_const_time_memcmp()`:
```c
int pin_const_time_memcmp(const void *a, const void *b, size_t len) {
    const uint8_t *ua = (const uint8_t *)a;
    const uint8_t *ub = (const uint8_t *)b;
    uint8_t diff = 0;
    for (size_t i = 0; i < len; ++i) {
        diff |= (ua[i] ^ ub[i]);
    }
    return (int)diff;
}
```

---

## 3. Network Transport & Lack of TLS

- **No Transport Layer Security (TLS):** Because constraint **C2** restricts dependencies strictly to the C11 standard library and native OS socket headers, TLS cannot be natively implemented without OpenSSL, mbedTLS, or OS-level Schannel/SecureTransport.
- **Threat Model:** The network protocol (ATMP/1) operates in plaintext. Although PINs are never transmitted in cleartext (using challenge-response HMAC), an eavesdropper on an unencrypted network could theoretically observe session tokens, account IDs, and transaction amounts.
- **Recommendation:** `bankd` defaults to binding on `127.0.0.1` (localhost). A clear warning is emitted whenever a non-loopback bind address is configured. In production, this traffic would be wrapped inside a TLS tunnel (e.g., stunnel, WireGuard, or reverse proxy).

---

## 4. Tamper-Proof Cryptographic Audit Log (F7)

All security-relevant actions are recorded in an append-only hash chain in `data/audit.log`.

### 4.1 Chain Structure
Each entry is hashed using SHA-256 over its predecessors and fields:
$$\text{hash}_n = \text{SHA256}(\text{hash}_{n-1} \mathbin{\Vert} \text{seq} \mathbin{\Vert} \text{timestamp} \mathbin{\Vert} \text{actor} \mathbin{\Vert} \text{action} \mathbin{\Vert} \text{target} \mathbin{\Vert} \text{detail})$$
- The genesis entry uses 32 zero bytes as `prev_hash`.
- The current head hash is saved inside the binary database (`bank.dat`) on every state change and printed when an administrator logs out.

### 4.2 Tamper Detection
- Modifying any byte within a historic log line alters that line's hash and breaks the link to every subsequent entry.
- Truncating or deleting trailing entries causes the computed chain head to disagree with the head hash stored in `bank.dat`.
- Verification (`atm --verify-audit`) traverses the entire log, reporting the exact broken sequence number.

---

## 5. Entropy & Random Number Generation (`rng.c`)

- On POSIX platforms, `rng_bytes()` reads directly from `/dev/urandom`.
- On Windows, cryptographic entropy is collected via `BCryptGenRandom` or falls back to a 64-bit XorShift generator seeded with system timestamps, high-resolution clocks, and process stack addresses.
