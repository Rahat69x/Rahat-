# ATMP/1 Network Protocol Specification

## 1. Overview

The **ATMP/1** (ATM Protocol Version 1) is a lightweight, line-oriented, tab-separated application layer protocol designed for communication between terminal ATM clients (`atm`) and the central banking daemon (`bankd`).

It operates over reliable byte-stream transports (TCP/IP) and provides:
- Bounded framing (maximum line length: 512 bytes).
- Challenge-response authentication without transmitting customer PINs in plaintext.
- Strict per-session request idempotency via monotonic sequence numbers (`req_id`).
- Session isolation with automatic idle timeout (`session_idle_s`).

---

## 2. Framing & Transport

- **Encoding:** ASCII / UTF-8.
- **Delimiter:** Every message terminates with a single newline (`\n`). Carriage returns (`\r`) are stripped during parsing.
- **Field Separator:** Tab character (`\t`).
- **Maximum Length:** 512 bytes per line (including newline). Messages exceeding this bound are discarded.
- **Socket I/O:** Both client and server employ buffered, non-blocking / event-driven socket handling with loop-based partial write/read resolution.

---

## 3. Connection Lifecycle

### 3.1 Handshake
Immediately upon establishing a TCP connection, the client must transmit the handshake greeting:
```
HELLO\tATMP/1\n
```
The server responds with:
```
HELLO\tATMP/1\tOK\n
```
Any unexpected greeting results in immediate socket termination by the server.

---

## 4. Message Formats

### 4.1 Requests (`REQ`)
```
REQ\t<req_id>\t<OP>\t<arg1>[\t<arg2>[\t<arg3>[\t<arg4>]]]\n
```
- `<req_id>`: 64-bit unsigned integer, monotonically incremented by the client.
- `<OP>`: ASCII operation code (case-sensitive).
- `<argN>`: Positional arguments (maximum 128 bytes each).

### 4.2 Responses (`RES`)
```
RES\t<req_id>\t<code>[\t<payload>]\n
```
- `<req_id>`: Mirrors the request ID for tracking and idempotency matching.
- `<code>`: Numeric status code:
  - `200`: Success.
  - `400`: Bad / Malformed Request.
  - `401`: Unauthorized / Authentication Failure.
  - `403`: Forbidden (e.g., Blocked / Frozen account).
  - `404`: Not Found.
  - `405`: Unsupported Operation.
  - `500`: Internal Server Error.
  - `503`: Service Unavailable.
- `<payload>`: Tab-separated payload values or human-readable error explanation.

---

## 5. Operations & Authentication Flow

### 5.1 Challenge-Response Authentication (Zero-Knowledge PIN)
To prevent cleartext PIN exposure across the socket, authentication uses HMAC-SHA256 challenge-response:

```
 Client                                              Server (bankd)
   |                                                       |
   |--- REQ <id> CHALLENGE <account_id> ------------------>|
   |<-- RES <id> 200 <salt_hex>\t<iter>\t<nonce_hex> ------|
   |                                                       |
   | [Derive pin_hash = SHA256^iter(pin, salt)]           |
   | [Compute hmac = HMAC_SHA256(pin_hash, nonce)]         |
   |                                                       |
   |--- REQ <id> LOGIN <account_id> <hmac_hex> ----------->|
   |                                                       | [Verify HMAC in const-time]
   |                                                       | [Generate 128-bit session token]
   |<-- RES <id> 200 <token>\t<name>\t<must_change> -------|
```

### 5.2 Banking Operations
Once authenticated, requests include the session token:

1. **Balance Inquiry:**
   - Request: `REQ <id> BALANCE <token>`
   - Response: `RES <id> 200 <balance_minor>`
2. **Cash Deposit:**
   - Request: `REQ <id> DEPOSIT <token> <amount_minor>`
   - Response: `RES <id> 200 <balance_after_minor>\t<receipt_id>`
3. **Cash Withdrawal:**
   - Request: `REQ <id> WITHDRAW <token> <amount_minor>`
   - Response: `RES <id> 200 <balance_after_minor>\t<receipt_id>`
4. **Session Logout:**
   - Request: `REQ <id> LOGOUT <token>`
   - Response: `RES <id> 200 LOGGED_OUT`

---

## 6. Idempotency Guarantees

Every client session maintains `last_req_id` and cached `last_response`. If a client re-submits a request with the same `req_id` (e.g., following a network timeout or retransmission):
1. The server identifies `req_id == cs->last_req_id`.
2. The server re-transmits the cached response directly from memory.
3. No internal state or account balance is debited or modified twice.
