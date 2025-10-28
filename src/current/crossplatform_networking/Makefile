# Project configuration
LIBRARY_NAME = crossplatform_networking
TARGET_STATIC = lib$(LIBRARY_NAME).a

# Source files
SRC_DIR = .
NETWORKING_SRCS = $(SRC_DIR)/crossplatform_networking.c
SIGNAL_SRCS = $(SRC_DIR)/crossplatform_signalhandling.c
THREADING_SRCS = $(SRC_DIR)/crossplatform_threading.c
SRCS = $(NETWORKING_SRCS) $(SIGNAL_SRCS) $(THREADING_SRCS)
OBJS = $(SRCS:.c=.o)

# Test applications
TEST_SERVER = test_server
TEST_CLIENT = test_client
TEST_SIGNAL = test_signal
TEST_THREADING = test_threading

# Compiler and flags
CC = gcc
CFLAGS = -Wall -Wextra -pedantic -std=c99 -fPIC
LDFLAGS = 

# Platform detection
UNAME_S := $(shell uname -s)

# Windows (MinGW) specific
ifneq ($(findstring MINGW,$(UNAME_S)),)
    TARGET_STATIC = lib$(LIBRARY_NAME).a
    TEST_SERVER = test_server.exe
    TEST_CLIENT = test_client.exe
    TEST_SIGNAL = test_signal.exe
	TEST_THREADING = test_threading.exe
    CFLAGS += -D_WIN32_WINNT=0x0600
    LDFLAGS = -lws2_32 -lpthread
    RM = rm -f
# Unix-like systems
else
    LDFLAGS = -lpthread
    RM = rm -f
endif

# Default target
all: static tests signal_test thread_test

# Static library
static: $(TARGET_STATIC)

# Test applications
tests: $(TEST_SERVER) $(TEST_CLIENT)

# Signal test
signal_test: $(TEST_SIGNAL)
thread_test: $(TEST_THREADING)

# Create static library
$(TARGET_STATIC): $(OBJS)
	$(AR) rcs $@ $^

# Test applications
$(TEST_SERVER): test_server.c $(TARGET_STATIC)
	$(CC) $(CFLAGS) -o $@ test_server.c -L. -l$(LIBRARY_NAME) $(LDFLAGS)

$(TEST_CLIENT): test_client.c $(TARGET_STATIC)
	$(CC) $(CFLAGS) -o $@ test_client.c -L. -l$(LIBRARY_NAME) $(LDFLAGS)

$(TEST_SIGNAL): test_signal.c $(TARGET_STATIC)
	$(CC) $(CFLAGS) -o $@ test_signal.c -L. -l$(LIBRARY_NAME) $(LDFLAGS)

$(TEST_THREADING): test_threading.c $(TARGET_STATIC)
	$(CC) $(CFLAGS) -o $@ test_threading.c -L. -l$(LIBRARY_NAME) $(LDFLAGS)
	
# Compile source files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Clean
clean:
	$(RM) $(OBJS) $(TARGET_STATIC) $(TEST_SERVER) $(TEST_CLIENT) $(TEST_SIGNAL) $(TEST_THREADING)

# Test signal handling
test_signal: $(TEST_SIGNAL)
	@echo "Running signal test (press Ctrl+C to test)..."
	./$(TEST_SIGNAL)

.PHONY: all static tests signal_test clean test_signal thread_test