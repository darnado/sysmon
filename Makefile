# Makefile for sysmon
#
# Targets:
#   make          — build the sysmon binary (default)
#   make clean    — remove build artefacts
#   make test     — compile and run the unit tests
#   make install  — install to $(PREFIX)/bin (default: /usr/local)
#
# Variables you can override on the command line:
#   CC=clang make
#   PREFIX=/usr make install

CC      ?= gcc
PREFIX  ?= /usr/local

# -std=c99       : POSIX extensions need _POSIX_C_SOURCE; c99 is a safe base
# -D_POSIX_C_SOURCE=200809L : expose POSIX.1-2008 APIs (opendir, sigaction, …)
# -Wall -Wextra  : catch common mistakes
# -Wpedantic     : strict standard compliance
# -g             : include debug symbols (harmless in release builds for a tool)
CFLAGS  := -std=c99 -D_POSIX_C_SOURCE=200809L \
           -Wall -Wextra -Wpedantic -g \
           -Iinclude

SRCS    := src/cpu.c src/mem.c src/proc.c src/display.c src/main.c
OBJS    := $(SRCS:.c=.o)
TARGET  := sysmon

TEST_SRCS  := tests/test_main.c tests/test_cpu.c tests/test_mem.c src/cpu.c src/mem.c
TEST_TARGET := tests/run_tests

.PHONY: all clean test install

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

# Pattern rule: compile each .c to a .o beside it
%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(OBJS) $(TARGET) $(TEST_TARGET) tests/*.o

test: $(TEST_TARGET)
	./$(TEST_TARGET)

$(TEST_TARGET): $(TEST_SRCS)
	$(CC) $(CFLAGS) -o $@ $^

install: $(TARGET)
	install -d $(DESTDIR)$(PREFIX)/bin
	install -m 755 $(TARGET) $(DESTDIR)$(PREFIX)/bin/$(TARGET)
