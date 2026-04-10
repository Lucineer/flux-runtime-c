CC=gcc
CFLAGS=-std=c11 -Wall -Wextra -O2 -Iinclude
OBJS=src/opcodes.o src/registers.o src/memory.o src/vm.o

all: flux-runtime test_vm test_memory

flux-runtime: src/main.c $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ -lm

test_vm: tests/test_vm.c $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ -lm

test_memory: tests/test_memory.c $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ -lm

src/%.o: src/%.c
	$(CC) $(CFLAGS) -c -o $@ $<

test: test_vm test_memory
	@echo "--- VM Tests ---"
	@./test_vm
	@echo "--- Memory Tests ---"
	@./test_memory

clean:
	rm -f flux-runtime test_vm test_memory src/*.o

.PHONY: all test clean
