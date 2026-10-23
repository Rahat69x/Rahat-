# Binary Storage & Persistence Format Specification

## 1. Overview

Persistence in `c-atm-simulation` is handled through two deterministic, platform-independent binary database files stored in the `data/` directory:
- `data/bank.dat`: Primary database storing accounts, administrators, vouchers, loans, standing orders, security alerts, and system state counters.
- `data/txns.dat`: Append-only transaction journal storing full historical ledgers and audit records.

---

## 2. File Header Layout

All binary files begin with a standard 32-byte header:

| Offset | Field Name | Type | Description |
|--------|------------|------|-------------|
| 0x00 | `magic` | `char[4]` | ASCII Magic identifier: `'A', 'T', 'M', 'C'` (`0x434D5441`) |
| 0x04 | `version` | `uint32_t` | File format version (currently `1`) |
| 0x08 | `record_count` | `uint32_t` | Number of primary records encoded in payload |
| 0x0C | `crc32` | `uint32_t` | CRC-32 checksum of the payload bytes (ISO 3309 / ITU-T V.42) |
| 0x10 | `sim_day` | `uint32_t` | Simulated bank clock day counter |
| 0x14 | `reserved` | `uint8_t[12]` | Reserved padding bytes (zero-filled for future expansion) |

---

## 3. Serialization Rules (C4, F4)

1. **No Raw Struct Dumps:** Structs are never dumped directly via `fwrite(&struct, sizeof(struct), ...)` to eliminate memory layout, compiler padding, and endianness discrepancies across architectures.
2. **Explicit Little-Endian Encoding:** Every scalar integer (`uint16_t`, `uint32_t`, `uint64_t`, `int64_t`) is encoded byte-by-byte in little-endian order.
3. **Fixed-Width Strings:** Strings are written with fixed bounds, null-terminated, and zero-padded to their schema size.
4. **Money Precision:** Monetary values are serialized strictly as 64-bit signed integers (`int64_t`) in minor units (e.g., cents: $1.00 = 100).

---

## 4. Atomic Save & Fault Tolerance

To guarantee zero database corruption during sudden crashes or power loss, state-changing saves follow a multi-stage atomic write workflow:

```
[In-Memory Bank] 
       |
       v (1. Write payload & compute CRC-32)
   bank.dat.tmp
       |
       v (2. fflush() & fclose())
   [Committed to Disk]
       |
       v (3. Backup existing active database)
   bank.dat  ===>  bank.dat.bak
       |
       v (4. Atomic atomic replace via rename())
   bank.dat.tmp ===> bank.dat
```

### 4.1 Corruption Recovery & Fallback
On startup, `store_load_bank()` inspects `bank.dat`:
1. Magic check: Must equal `ATMC`.
2. Version check: Must match supported version (version migration hooks invoked if upgrade needed).
3. Payload Checksum: Recomputes CRC-32 over payload and compares with header `crc32`.
4. Fallback: If corruption or CRC mismatch is detected, the loader immediately logs a security warning and attempts recovery from `bank.dat.bak`. If both are corrupted, startup halts with a descriptive error.

---

## 5. Single-Writer Process Lock (`data/.lock`)

To prevent concurrent file access corruption across processes, `bankd` and standalone `atm` acquire an exclusive file lock:
- Created using `fopen("data/.lock", "wx")` (C11 atomic exclusive create mode).
- If the file exists, the program refuses to start and informs the operator:
  ```
  [FATAL] Database is locked by another running process (data/.lock exists).
  ```
- The lock file is cleanly removed on graceful process shutdown.

---

## 6. Versioning & Migration Architecture

The format version field enables forward and backward compatibility:
- Version 1: Initial schema with accounts, admins, vouchers, loans, standing orders, alerts.
- Stubs for `migrate_v1_to_v2()` are defined in `store.c` to facilitate seamless schema evolutions without data loss.
