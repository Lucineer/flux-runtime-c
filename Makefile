CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -Werror -O2 -Iinclude
LDFLAGS = -lm

.PHONY: all test clean

all: test_instinct test_extended test_final

test_final: tests/test_final.c src/flux_vm.c src/memory.c
	$(CC) $(CFLAGS) -o test_final tests/test_final.c src/flux_vm.c src/memory.c $(LDFLAGS)
	./test_final


test_extended: tests/test_extended.c src/flux_vm.c src/memory.c
	$(CC) $(CFLAGS) -o test_extended tests/test_extended.c src/flux_vm.c src/memory.c $(LDFLAGS)
	./test_extended


test_instinct: tests/test_instinct.c src/flux_vm.c
	$(CC) -o test_instinct tests/test_instinct.c -Iinclude -Isrc $(CFLAGS)
	./test_instinct
	test_flux_vm

test_flux_vm: tests/test_flux_vm.c src/flux_vm.c
	$(CC) $(CFLAGS) -o test_flux_vm tests/test_flux_vm.c src/flux_vm.c $(LDFLAGS)

test: test_flux_vm
	./test_flux_vm

clean:
	rm -f test_flux_vm
