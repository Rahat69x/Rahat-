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
    TEST_BUCKETS = $(BUILD_DIR)/test_buckets.exe
    TEST_QUEUE = $(BUILD_DIR)/test_queue.exe
    TEST_HEAP = $(BUILD_DIR)/test_heap.exe
    TEST_REQ_SRV = $(BUILD_DIR)/test_request_service.exe
    MKDIR = if not exist $(BUILD_DIR) mkdir $(BUILD_DIR)
    RM = if exist $(BUILD_DIR) rd /s /q $(BUILD_DIR)
    RUN_TEST_PHASE0 = $(TEST_PHASE0)
    RUN_TEST_DONOR = $(TEST_DONOR)
    RUN_TEST_BST = $(TEST_BST)
    RUN_TEST_HT = $(TEST_HT)
    RUN_TEST_BUCKETS = $(TEST_BUCKETS)
    RUN_TEST_QUEUE = $(TEST_QUEUE)
    RUN_TEST_HEAP = $(TEST_HEAP)
    RUN_TEST_REQ_SRV = $(TEST_REQ_SRV)
else
    TARGET = $(BUILD_DIR)/bgms
    TEST_PHASE0 = $(BUILD_DIR)/test_phase0
    TEST_DONOR = $(BUILD_DIR)/test_donor
    TEST_BST = $(BUILD_DIR)/test_bst
    TEST_HT = $(BUILD_DIR)/test_hashtable
    TEST_BUCKETS = $(BUILD_DIR)/test_buckets
    TEST_QUEUE = $(BUILD_DIR)/test_queue
    TEST_HEAP = $(BUILD_DIR)/test_heap
    TEST_REQ_SRV = $(BUILD_DIR)/test_request_service
    MKDIR = mkdir -p $(BUILD_DIR)
    RM = rm -rf $(BUILD_DIR)
    RUN_TEST_PHASE0 = ./$(TEST_PHASE0)
    RUN_TEST_DONOR = ./$(TEST_DONOR)
    RUN_TEST_BST = ./$(TEST_BST)
    RUN_TEST_HT = ./$(TEST_HT)
    RUN_TEST_BUCKETS = ./$(TEST_BUCKETS)
    RUN_TEST_QUEUE = ./$(TEST_QUEUE)
    RUN_TEST_HEAP = ./$(TEST_HEAP)
    RUN_TEST_REQ_SRV = ./$(TEST_REQ_SRV)
endif

# Core Sources & Headers
SRCS = common.c utils.c date.c bloodgroup.c donor.c bst.c hashtable.c buckets.c request.c queue.c heap.c context.c donor_service.c request_service.c
OBJS = $(patsubst %.c,$(BUILD_DIR)/%.o,$(SRCS))
HEADERS = common.h utils.h date.h bloodgroup.h donor.h bst.h hashtable.h buckets.h request.h queue.h heap.h context.h donor_service.h request_service.h

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

$(TEST_BUCKETS): $(OBJS) tests/test_buckets.c tests/testing.h $(HEADERS) | dirs
	$(CC) $(CFLAGS) $(INCLUDES) $(OBJS) tests/test_buckets.c -o $@

$(TEST_QUEUE): $(OBJS) tests/test_queue.c tests/testing.h $(HEADERS) | dirs
	$(CC) $(CFLAGS) $(INCLUDES) $(OBJS) tests/test_queue.c -o $@

$(TEST_HEAP): $(OBJS) tests/test_heap.c tests/testing.h $(HEADERS) | dirs
	$(CC) $(CFLAGS) $(INCLUDES) $(OBJS) tests/test_heap.c -o $@

$(TEST_REQ_SRV): $(OBJS) tests/test_request_service.c tests/testing.h $(HEADERS) | dirs
	$(CC) $(CFLAGS) $(INCLUDES) $(OBJS) tests/test_request_service.c -o $@

test: dirs $(TEST_PHASE0) $(TEST_DONOR) $(TEST_BST) $(TEST_HT) $(TEST_BUCKETS) $(TEST_QUEUE) $(TEST_HEAP) $(TEST_REQ_SRV)
	@echo "--- Running Phase 0 Unit Tests ---"
	$(RUN_TEST_PHASE0)
	@echo "--- Running Phase 1 Donor Unit Tests ---"
	$(RUN_TEST_DONOR)
	@echo "--- Running Phase 2 BST Unit Tests ---"
	$(RUN_TEST_BST)
	@echo "--- Running Phase 3 Hash Table Unit Tests ---"
	$(RUN_TEST_HT)
	@echo "--- Running Phase 4 Buckets Unit Tests ---"
	$(RUN_TEST_BUCKETS)
	@echo "--- Running Phase 5 Queue Unit Tests ---"
	$(RUN_TEST_QUEUE)
	@echo "--- Running Phase 6 Heap Unit Tests ---"
	$(RUN_TEST_HEAP)
	@echo "--- Running Phase 7 Request Service Unit Tests ---"
	$(RUN_TEST_REQ_SRV)

clean:
	@$(RM)
