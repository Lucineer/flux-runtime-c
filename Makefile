CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -O2 -Iinclude
SRCS = src/opcodes.c src/registers.c src/memory.c src/vm.c
OBJS = build/opcodes.o build/registers.o build/memory.o build/vm.o
TEST_VM_OBJS = $(OBJS) build/test_vm.o
TEST_MEM_OBJS = $(OBJS) build/test_memory.o
MAIN_OBJS = $(OBJS) build/main.o

.PHONY: all test clean

all: build/flux

build:
	mkdir -p build

build/%.o: src/%.c | build
	$(CC) $(CFLAGS) -c $< -o $@

build/%.o: tests/%.c | build
	$(CC) $(CFLAGS) -c $< -o $@

build/flux: $(MAIN_OBJS)
	$(CC) $(CFLAGS) $^ -o $@ -lm

build/test_vm: $(TEST_VM_OBJS)
	$(CC) $(CFLAGS) $^ -o $@ -lm

build/test_memory: $(TEST_MEM_OBJS)
	$(CC) $(CFLAGS) $^ -o $@ -lm

test: build/test_vm build/test_memory
	@echo "=== VM Tests ==="
	./build/test_vm
	@echo "=== Memory Tests ==="
	./build/test_memory

clean:
	rm -rf build
