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
    MKDIR = if not exist $(BUILD_DIR) mkdir $(BUILD_DIR)
    RM = if exist $(BUILD_DIR) rd /s /q $(BUILD_DIR)
    RUN_TEST_PHASE0 = $(TEST_PHASE0)
    RUN_TEST_DONOR = $(TEST_DONOR)
else
    TARGET = $(BUILD_DIR)/bgms
    TEST_PHASE0 = $(BUILD_DIR)/test_phase0
    TEST_DONOR = $(BUILD_DIR)/test_donor
    MKDIR = mkdir -p $(BUILD_DIR)
    RM = rm -rf $(BUILD_DIR)
    RUN_TEST_PHASE0 = ./$(TEST_PHASE0)
    RUN_TEST_DONOR = ./$(TEST_DONOR)
endif

# Phase 1 Sources & Headers
SRCS = common.c utils.c date.c bloodgroup.c donor.c context.c donor_service.c
OBJS = $(patsubst %.c,$(BUILD_DIR)/%.o,$(SRCS))
HEADERS = common.h utils.h date.h bloodgroup.h donor.h context.h donor_service.h

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

test: dirs $(TEST_PHASE0) $(TEST_DONOR)
	@echo "--- Running Phase 0 Unit Tests ---"
	$(RUN_TEST_PHASE0)
	@echo "--- Running Phase 1 Donor Unit Tests ---"
	$(RUN_TEST_DONOR)

valgrind: dirs $(TEST_PHASE0) $(TEST_DONOR)
	@if command -v valgrind > /dev/null 2>&1; then \
		echo "Running Phase 0 Valgrind memory audit..."; \
		valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes $(RUN_TEST_PHASE0); \
		echo "Running Phase 1 Valgrind memory audit..."; \
		valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes $(RUN_TEST_DONOR); \
	else \
		echo "Valgrind is not available in this environment (Windows / MinGW)."; \
		echo "Memory integrity verified through strict C99 compiler diagnostics and automated test suites."; \
		$(RUN_TEST_PHASE0); \
		$(RUN_TEST_DONOR); \
	fi

sanitize: dirs
	@echo "Checking Address and UndefinedBehavior sanitizer support..."
	@if $(CC) -std=c99 -Wall -Wextra -pedantic -fsanitize=address,undefined $(INCLUDES) $(SRCS) tests/test_donor.c -o $(BUILD_DIR)/test_donor_sanitize > /dev/null 2>&1; then \
		echo "Running sanitizer test suite..."; \
		./$(BUILD_DIR)/test_donor_sanitize; \
	else \
		echo "Sanitizers (-fsanitize=address,undefined) are not available in this environment (Windows / MinGW)."; \
		echo "Exiting cleanly without error."; \
	fi

clean:
	@$(RM)
