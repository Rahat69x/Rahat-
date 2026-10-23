# ==============================================================================
# c-atm-simulation Makefile
# Strict C11 Standard: -std=c11 -Wall -Wextra -Wpedantic -Werror
# ==============================================================================

CC ?= gcc
CFLAGS = -std=c11 -Wall -Wextra -Wpedantic -Werror -Iinclude
LDFLAGS = -lm

SRC_CORE = src/core/bank.c src/core/account.c src/core/txn.c src/core/money.c src/core/clock.c
SRC_SEC  = src/security/sha256.c src/security/hmac.c src/security/crc32.c src/security/rng.c src/security/pin.c
SRC_STOR = src/storage/config.c
SRC_UI   = src/ui/input.c src/ui/render.c src/ui/menu_customer.c

ALL_OBJS = $(SRC_CORE) $(SRC_SEC) $(SRC_STOR) $(SRC_UI)

TARGET_ATM = atm.exe
TARGET_BANKD = bankd.exe
TEST_CRYPTO = test_crypto.exe
TEST_REGRESSION = test_regression.exe

.PHONY: all test asan clean run-atm run-bankd

all: $(TARGET_ATM)

$(TARGET_ATM): src/main_atm.c $(ALL_OBJS)
	$(CC) $(CFLAGS) -O2 src/main_atm.c $(ALL_OBJS) -o $(TARGET_ATM) $(LDFLAGS)

$(TEST_CRYPTO): tests/unit/test_f7_crypto.c $(SRC_SEC)
	$(CC) $(CFLAGS) -O2 tests/unit/test_f7_crypto.c $(SRC_SEC) -o $(TEST_CRYPTO) $(LDFLAGS)

$(TEST_REGRESSION): tests/regression/test_baseline.c $(ALL_OBJS)
	$(CC) $(CFLAGS) -O2 tests/regression/test_baseline.c $(ALL_OBJS) -o $(TEST_REGRESSION) $(LDFLAGS)

test: $(TEST_CRYPTO) $(TEST_REGRESSION)
	@echo "[*] Running F7 Cryptographic Primitives Test Suite..."
	.\$(TEST_CRYPTO)
	@echo "[*] Running Baseline ATM Regression Test Suite..."
	.\$(TEST_REGRESSION)

asan: CFLAGS += -fsanitize=address,undefined -g
asan: clean test

clean:
	rm -f $(TARGET_ATM) $(TARGET_BANKD) $(TEST_CRYPTO) $(TEST_REGRESSION) *.o src/**/*.o tests/**/*.o data/*.lock data/*.tmp 2>/dev/null || true

run-atm: $(TARGET_ATM)
	.\$(TARGET_ATM) --standalone

run-bankd:
	@echo "[INFO] bankd will be compiled in Step 4"
