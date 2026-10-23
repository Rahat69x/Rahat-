# ==============================================================================
# c-atm-simulation Makefile
# Strict C11 Standard: -std=c11 -Wall -Wextra -Wpedantic -Werror
# ==============================================================================

CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -Wpedantic -Werror -Iinclude
LDFLAGS = -lm

SRC_CORE = src/core/bank.c src/core/account.c src/core/txn.c src/core/money.c src/core/clock.c
SRC_CRYPTO = src/security/sha256.c src/security/hmac.c src/security/crc32.c src/security/rng.c src/security/pin.c
SRC_SEC  = $(SRC_CRYPTO) src/security/audit.c
SRC_STOR = src/storage/config.c src/storage/store.c
SRC_FEAT = src/features/admin.c
SRC_UI   = src/ui/input.c src/ui/render.c src/ui/menu_customer.c src/ui/menu_admin.c

ALL_OBJS = $(SRC_CORE) $(SRC_SEC) $(SRC_STOR) $(SRC_FEAT) $(SRC_UI)

TARGET_ATM = atm.exe
TARGET_BANKD = bankd.exe
TEST_CRYPTO = test_crypto.exe
TEST_REGRESSION = test_regression.exe
TEST_PERSIST = test_persistence.exe
TEST_AUDIT = test_audit.exe
TEST_ADMIN = test_admin.exe

.PHONY: all test asan clean run-atm run-bankd

all: $(TARGET_ATM)

$(TARGET_ATM): src/main_atm.c $(ALL_OBJS)
	$(CC) $(CFLAGS) -O2 src/main_atm.c $(ALL_OBJS) -o $(TARGET_ATM) $(LDFLAGS)

$(TEST_CRYPTO): tests/unit/test_f7_crypto.c $(SRC_CRYPTO)
	$(CC) $(CFLAGS) -O2 tests/unit/test_f7_crypto.c $(SRC_CRYPTO) -o $(TEST_CRYPTO) $(LDFLAGS)

$(TEST_REGRESSION): tests/regression/test_baseline.c $(ALL_OBJS)
	$(CC) $(CFLAGS) -O2 tests/regression/test_baseline.c $(ALL_OBJS) -o $(TEST_REGRESSION) $(LDFLAGS)

$(TEST_PERSIST): tests/unit/test_f4_persistence.c $(ALL_OBJS)
	$(CC) $(CFLAGS) -O2 tests/unit/test_f4_persistence.c $(ALL_OBJS) -o $(TEST_PERSIST) $(LDFLAGS)

$(TEST_AUDIT): tests/unit/test_f7_audit.c $(ALL_OBJS)
	$(CC) $(CFLAGS) -O2 tests/unit/test_f7_audit.c $(ALL_OBJS) -o $(TEST_AUDIT) $(LDFLAGS)

$(TEST_ADMIN): tests/unit/test_f3_admin.c $(ALL_OBJS)
	$(CC) $(CFLAGS) -O2 tests/unit/test_f3_admin.c $(ALL_OBJS) -o $(TEST_ADMIN) $(LDFLAGS)

test: $(TEST_CRYPTO) $(TEST_REGRESSION) $(TEST_PERSIST) $(TEST_AUDIT) $(TEST_ADMIN)
	@echo "[*] Running F7 Cryptographic Primitives Test Suite..."
	.\$(TEST_CRYPTO)
	@echo "[*] Running Baseline ATM Regression Test Suite..."
	.\$(TEST_REGRESSION)
	@echo "[*] Running F4 Persistence Test Suite..."
	.\$(TEST_PERSIST)
	@echo "[*] Running F7 Audit Log Integrity Test Suite..."
	.\$(TEST_AUDIT)
	@echo "[*] Running F3 Admin Accounts Test Suite..."
	.\$(TEST_ADMIN)

asan: CFLAGS += -fsanitize=address,undefined -g
asan: clean test

clean:
	rm -f $(TARGET_ATM) $(TARGET_BANKD) $(TEST_CRYPTO) $(TEST_REGRESSION) $(TEST_PERSIST) $(TEST_AUDIT) $(TEST_ADMIN) *.o src/**/*.o tests/**/*.o data/*.lock data/*.tmp data/*.bak 2>/dev/null || true

run-atm: $(TARGET_ATM)
	.\$(TARGET_ATM) --standalone

run-bankd:
	@echo "[INFO] bankd will be compiled in Step 4"
