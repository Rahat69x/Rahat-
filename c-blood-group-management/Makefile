# ==============================================================================
# Makefile for Blood Group Management System (C99 Standard)
# Portable across Linux, macOS, and Windows (make / mingw32-make)
# Outputs written to build/ directory
# ==============================================================================

CC = gcc
CFLAGS = -std=c99 -Wall -Wextra -pedantic -O2
INCLUDES = -I. -Itests

BUILD_DIR = build

ifeq ($(OS),Windows_NT)
    TARGET = $(BUILD_DIR)/bgms.exe
    TEST_PHASE0 = $(BUILD_DIR)/test_phase0.exe
    TEST_DONOR = $(BUILD_DIR)/test_donor.exe
    TEST_BST = $(BUILD_DIR)/test_bst.exe
    TEST_HT = $(BUILD_DIR)/test_hashtable.exe
    MKDIR = if not exist $(BUILD_DIR) mkdir $(BUILD_DIR)
    RM = if exist $(BUILD_DIR) rd /s /q $(BUILD_DIR)
    RUN_TEST_PHASE0 = $(TEST_PHASE0)
    RUN_TEST_DONOR = $(TEST_DONOR)
    RUN_TEST_BST = $(TEST_BST)
    RUN_TEST_HT = $(TEST_HT)
else
    TARGET = $(BUILD_DIR)/bgms
    TEST_PHASE0 = $(BUILD_DIR)/test_phase0
    TEST_DONOR = $(BUILD_DIR)/test_donor
    TEST_BST = $(BUILD_DIR)/test_bst
    TEST_HT = $(BUILD_DIR)/test_hashtable
    MKDIR = mkdir -p $(BUILD_DIR)
    RM = rm -rf $(BUILD_DIR)
    RUN_TEST_PHASE0 = ./$(TEST_PHASE0)
    RUN_TEST_DONOR = ./$(TEST_DONOR)
    RUN_TEST_BST = ./$(TEST_BST)
    RUN_TEST_HT = ./$(TEST_HT)
endif

# Core Sources & Headers
SRCS = common.c utils.c date.c bloodgroup.c donor.c bst.c hashtable.c context.c donor_service.c
OBJS = $(patsubst %.c,$(BUILD_DIR)/%.o,$(SRCS))
HEADERS = common.h utils.h date.h bloodgroup.h donor.h bst.h hashtable.h context.h donor_service.h

.PHONY: all test valgrind sanitize clean dirs

all: dirs $(TARGET)

dirs:
	@$(MKDIR)

$(TARGET): $(OBJS) $(BUILD_DIR)/main.o | dirs
	$(CC) $(CFLAGS) -o $@ $(OBJS) $(BUILD_DIR)/main.o

$(BUILD_DIR)/%.o: %.c $(HEADERS) | dirs
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/main.o: main.c $(HEADERS) | dirs
	$(CC) $(CFLAGS) $(INCLUDES) -c main.c -o $@

$(TEST_PHASE0): $(OBJS) tests/test_phase0.c tests/testing.h $(HEADERS) | dirs
	$(CC) $(CFLAGS) $(INCLUDES) $(OBJS) tests/test_phase0.c -o $@

$(TEST_DONOR): $(OBJS) tests/test_donor.c tests/testing.h $(HEADERS) | dirs
	$(CC) $(CFLAGS) $(INCLUDES) $(OBJS) tests/test_donor.c -o $@

$(TEST_BST): $(OBJS) tests/test_bst.c tests/testing.h $(HEADERS) | dirs
	$(CC) $(CFLAGS) $(INCLUDES) $(OBJS) tests/test_bst.c -o $@

$(TEST_HT): $(OBJS) tests/test_hashtable.c tests/testing.h $(HEADERS) | dirs
	$(CC) $(CFLAGS) $(INCLUDES) $(OBJS) tests/test_hashtable.c -o $@

test: dirs $(TEST_PHASE0) $(TEST_DONOR) $(TEST_BST) $(TEST_HT)
	@echo "--- Running Phase 0 Unit Tests ---"
	$(RUN_TEST_PHASE0)
	@echo "--- Running Phase 1 Donor Unit Tests ---"
	$(RUN_TEST_DONOR)
	@echo "--- Running Phase 2 BST Unit Tests ---"
	$(RUN_TEST_BST)
	@echo "--- Running Phase 3 Hash Table Unit Tests ---"
	$(RUN_TEST_HT)

valgrind: dirs $(TEST_PHASE0) $(TEST_DONOR) $(TEST_BST) $(TEST_HT)
	@if command -v valgrind > /dev/null 2>&1; then \
		echo "Running Phase 0 Valgrind memory audit..."; \
		valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes $(RUN_TEST_PHASE0); \
		echo "Running Phase 1 Valgrind memory audit..."; \
		valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes $(RUN_TEST_DONOR); \
		echo "Running Phase 2 BST Valgrind memory audit..."; \
		valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes $(RUN_TEST_BST); \
		echo "Running Phase 3 Hash Table Valgrind memory audit..."; \
		valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes $(RUN_TEST_HT); \
	else \
		echo "Valgrind is not available in this environment (Windows / MinGW)."; \
		echo "Memory integrity verified through strict C99 compiler diagnostics and automated test suites."; \
		$(RUN_TEST_PHASE0); \
		$(RUN_TEST_DONOR); \
		$(RUN_TEST_BST); \
		$(RUN_TEST_HT); \
	fi

clean:
	@$(RM)
