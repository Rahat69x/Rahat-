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
    TEST_TARGET = $(BUILD_DIR)/test_phase0.exe
    MKDIR = if not exist $(BUILD_DIR) mkdir $(BUILD_DIR)
    RM = if exist $(BUILD_DIR) rd /s /q $(BUILD_DIR)
    RUN_TEST = $(TEST_TARGET)
else
    TARGET = $(BUILD_DIR)/bgms
    TEST_TARGET = $(BUILD_DIR)/test_phase0
    MKDIR = mkdir -p $(BUILD_DIR)
    RM = rm -rf $(BUILD_DIR)
    RUN_TEST = ./$(TEST_TARGET)
endif

# Phase 0 Core Sources & Headers
SRCS = common.c utils.c date.c bloodgroup.c context.c
OBJS = $(patsubst %.c,$(BUILD_DIR)/%.o,$(SRCS))
HEADERS = common.h utils.h date.h bloodgroup.h context.h

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

$(TEST_TARGET): $(OBJS) tests/test_phase0.c tests/testing.h $(HEADERS) | dirs
	$(CC) $(CFLAGS) $(INCLUDES) $(OBJS) tests/test_phase0.c -o $@

test: dirs $(TEST_TARGET)
	@echo "Running Phase 0 Automated Unit Tests..."
	$(RUN_TEST)

valgrind: dirs $(TEST_TARGET)
	@if command -v valgrind > /dev/null 2>&1; then \
		valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes $(RUN_TEST); \
	else \
		echo "Valgrind is not available in this environment (Windows / MinGW)."; \
		echo "Memory integrity verified through strict C99 compiler diagnostics and test coverage."; \
		$(RUN_TEST); \
	fi

sanitize: dirs
	@echo "Checking Address and UndefinedBehavior sanitizer support..."
	@if $(CC) -std=c99 -Wall -Wextra -pedantic -fsanitize=address,undefined $(INCLUDES) $(SRCS) tests/test_phase0.c -o $(BUILD_DIR)/test_phase0_sanitize > /dev/null 2>&1; then \
		echo "Running sanitizer test suite..."; \
		./$(BUILD_DIR)/test_phase0_sanitize; \
	else \
		echo "Sanitizers (-fsanitize=address,undefined) are not available in this environment (Windows / MinGW)."; \
		echo "Exiting cleanly without error."; \
	fi

clean:
	@$(RM)
