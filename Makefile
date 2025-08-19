# Simple Makefile with AddressSanitizer (ASan) and macOS leaks checks

CC := clang
TARGET := httpserver
SRCDIR = src
APP_DIR = $(SRCDIR)/app

# All C files in the project
SOURCES := $(wildcard $(SRCDIR)/*.c) $(wildcard $(APP_DIR)/*.c)

# Base debug flags
# -Isrc: Add src directory to include paths for headers like "app/http.h"
CFLAGS := -g -O0 -Wall -Wextra -Wpedantic -I$(SRCDIR)
LDFLAGS :=
LDLIBS := -lpthread -lz

# Sanitizers
SANITIZE := -fsanitize=address,undefined -fno-omit-frame-pointer

# Platform detection for ASan runtime options
UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Darwin)
  # macOS system clang does not support LeakSanitizer; omit detect_leaks
  ASAN_RUN_ENV := ASAN_OPTIONS=abort_on_error=1,strict_string_checks=1,alloc_dealloc_mismatch=1,detect_stack_use_after_return=1
else
  ASAN_RUN_ENV := ASAN_OPTIONS=detect_leaks=1,abort_on_error=1,strict_string_checks=1,alloc_dealloc_mismatch=1,detect_stack_use_after_return=1
endif

# --- Testing Setup ---
TESTDIR = tests
TEST_SRCS := $(wildcard $(TESTDIR)/test_*.c)
# Exclude main.c from the app sources for testing
APP_SRCS_FOR_TESTS := $(filter-out $(SRCDIR)/main.c, $(SOURCES))
# CUnit for testing (requires `cunit` installed, e.g., via `brew install cunit`)
BREW_PREFIX := $(shell brew --prefix)
CUNIT_INC  = -I$(BREW_PREFIX)/include
CUNIT_LIB  = -L$(BREW_PREFIX)/lib -lcunit
TEST_CFLAGS = $(CFLAGS) $(CUNIT_INC)
TEST_LDFLAGS = $(LDFLAGS) $(CUNIT_LIB)


.PHONY: all build asan run run-asan check-leaks test clean help

all: build

build:
	$(CC) $(CFLAGS) $(SOURCES) -o $(TARGET) $(LDFLAGS) $(LDLIBS)

asan:
	$(CC) $(CFLAGS) $(SANITIZE) $(SOURCES) -o $(TARGET) $(LDFLAGS) $(LDLIBS)

run: build
	./$(TARGET)

# Run with ASan + LeakSanitizer enabled (AddressSanitizer on macOS includes leak detection)
run-asan: asan
	$(ASAN_RUN_ENV) ./$(TARGET)

# macOS 'leaks' tool: checks for leaks at process exit
check-leaks: build
	leaks --atExit -- ./$(TARGET)

# Build and run all tests
test:
	@for test_file in $(TEST_SRCS); do \
		test_runner=$${test_file%.c}; \
		echo "--- Building and running test: $$test_runner ---"; \
		$(CC) $(TEST_CFLAGS) -o $$test_runner $$test_file $(APP_SRCS_FOR_TESTS) $(TEST_LDFLAGS) $(LDLIBS) && ./$$test_runner; \
		echo ""; \
	done

clean:
	rm -f $(TARGET) $(patsubst %.c,%,$(TEST_SRCS))
	rm -f src/*.o src/app/*.o tests/*.o
	rm -f ./files/*

help:
	@echo "Targets:"
	@echo "  build       - Build with debug flags"
	@echo "  asan        - Build with AddressSanitizer + UBSan"
	@echo "  run         - Run debug build"
	@echo "  run-asan    - Run with ASan"
	@echo "  check-leaks - Run macOS leaks tool at exit"
	@echo "  test        - Build and run all CUnit tests"
	@echo "  clean       - Remove binary and test runners"

