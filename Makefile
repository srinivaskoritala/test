CC = gcc
CFLAGS = -Wall -Wextra -g -I./src -I/usr/local/include -I/opt/homebrew/include
LDFLAGS = -L/usr/local/lib -L/opt/homebrew/lib -lcurl -ljson-c -lmicrohttpd -lpthread

# macOS specific
UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Darwin)
    LDFLAGS += -framework CoreFoundation
endif

SRC_DIR = src
TEST_DIR = tests
BUILD_DIR = build

# Source files
SRCS = $(wildcard $(SRC_DIR)/*.c) $(wildcard $(SRC_DIR)/**/*.c)
OBJS = $(SRCS:$(SRC_DIR)/%.c=$(BUILD_DIR)/%.o)

# Test files
TEST_SRCS = $(wildcard $(TEST_DIR)/*.c)
TEST_OBJS = $(TEST_SRCS:$(TEST_DIR)/%.c=$(BUILD_DIR)/$(TEST_DIR)/%.o)
TEST_BINS = $(TEST_SRCS:$(TEST_DIR)/%.c=$(BUILD_DIR)/$(TEST_DIR)/%)

# Main executable
MAIN = $(BUILD_DIR)/network_service

.PHONY: all clean test

all: $(MAIN)

test: $(TEST_BINS)
	@for test in $(TEST_BINS); do \
		echo "Running $$test..."; \
		$$test; \
	done

$(MAIN): $(OBJS)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c -o $@ $<

$(BUILD_DIR)/$(TEST_DIR)/%: $(BUILD_DIR)/$(TEST_DIR)/%.o $(filter-out $(BUILD_DIR)/main.o,$(OBJS))
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(BUILD_DIR)/$(TEST_DIR)/%.o: $(TEST_DIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -rf $(BUILD_DIR) 