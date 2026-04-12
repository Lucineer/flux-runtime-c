#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "flux_vm.h"

#define PASS(name) printf("  PASS: %s\n", name)
#define FAIL(name, got, exp) printf("  FAIL: %s (got %d, expected %d)\n", name, got, exp)

int main(void) {
    int failures = 0;

    /* Test 1: IASLOAD — load from memory */
    {
        FluxVM vm;
        flux_vm_init(&vm);
        vm.memory[100] = 42;
        /* Format F: opcode rd imm16_lo imm16_hi */
        uint8_t bc[] = {0xB0, 0x01, 100, 0}; /* IASLOAD R1, 100 */
        flux_vm_execute(&vm, bc, 4);
        if (vm.gp[1] == 42) PASS("iasload");
        else { FAIL("iasload", vm.gp[1], 42); failures++; }
    }

    /* Test 2: IASTORE — store to memory */
    {
        FluxVM vm;
        flux_vm_init(&vm);
        vm.gp[2] = 99;
        uint8_t bc[] = {0xB1, 0x02, 200, 0}; /* IASTORE R2, 200 */
        flux_vm_execute(&vm, bc, 4);
        if (vm.memory[200] == 99) PASS("iastore");
        else { FAIL("iastore", vm.memory[200], 99); failures++; }
    }

    /* Test 3: IASDECAY — 95% decay */
    {
        FluxVM vm;
        flux_vm_init(&vm);
        vm.gp[3] = 1000;
        uint8_t bc[] = {0xB2, 0x03}; /* IASDECAY R3 */
        flux_vm_execute(&vm, bc, 2);
        if (vm.gp[3] == 950) PASS("iasdecay");
        else { FAIL("iasdecay", vm.gp[3], 950); failures++; }
    }

    /* Test 4: IASREFLEX — triggered (rd > 0, jump) */
    {
        FluxVM vm;
        flux_vm_init(&vm);
        vm.gp[4] = 50;
        /* Jump target: R5=777. Skip path: R5=999 then HALT. */
        uint8_t bc[] = {
            0xB3, 0x04, 9, 0,       /* IASREFLEX R4, addr=9 */
            0x40, 0x05, 0xE7, 0x03, /* MOVI16 R5, 999 (skip path) */
            0x00,                   /* HALT */
            0x40, 0x05, 0x09, 0x03, /* MOVI16 R5, 777 (jump target) */
            0x00                    /* HALT */
        };
        flux_vm_execute(&vm, bc, 14);
        if (vm.gp[5] == 777 && vm.gp[4] == 49) PASS("iasreflex_trigger");
        else { printf("  FAIL: iasreflex_trigger (R5=%d R4=%d)\n", vm.gp[5], vm.gp[4]); failures++; }
    }

    /* Test 5: IASREFLEX — not triggered (rd = 0) */
    {
        FluxVM vm;
        flux_vm_init(&vm);
        vm.gp[4] = 0;
        /* Skip path: R5=999 then HALT. Jump target: R5=777 */
        uint8_t bc[] = {
            0xB3, 0x04, 9, 0,       /* IASREFLEX R4, addr=9 */
            0x40, 0x05, 0xE7, 0x03, /* MOVI16 R5, 999 (skip path) */
            0x00,                   /* HALT (stop here on skip) */
            0x40, 0x05, 0x09, 0x03, /* MOVI16 R5, 777 (jump target) */
            0x00                    /* HALT (stop after jump) */
        };
        flux_vm_execute(&vm, bc, 14);
        if (vm.gp[5] == 999) PASS("iasreflex_skip");
        else { FAIL("iasreflex_skip", vm.gp[5], 999); failures++; }
    }

    /* Test 6: IASMODULATE — multiply by factor */
    {
        FluxVM vm;
        flux_vm_init(&vm);
        vm.gp[6] = 500;
        /* factor = 2000 -> rd * 2000 / 1000 = 1000 */
        uint8_t bc[] = {0xB4, 0x06, 0xD0, 0x07}; /* IASMODULATE R6, 2000 */
        flux_vm_execute(&vm, bc, 4);
        if (vm.gp[6] == 1000) PASS("iasmodulate");
        else { FAIL("iasmodulate", vm.gp[6], 1000); failures++; }
    }

    /* Test 7: IASTHRESHOLD — above */
    {
        FluxVM vm;
        flux_vm_init(&vm);
        vm.gp[7] = 100;
        uint8_t bc[] = {0xB5, 0x07, 0x50, 0x00}; /* IASTHRESHOLD R7, 80 */
        flux_vm_execute(&vm, bc, 4);
        if (vm.gp[7] == 1) PASS("iasthreshold_above");
        else { FAIL("iasthreshold_above", vm.gp[7], 1); failures++; }
    }

    /* Test 8: IASTHRESHOLD — below */
    {
        FluxVM vm;
        flux_vm_init(&vm);
        vm.gp[7] = 50;
        uint8_t bc[] = {0xB5, 0x07, 0x64, 0x00}; /* IASTHRESHOLD R7, 100 */
        flux_vm_execute(&vm, bc, 4);
        if (vm.gp[7] == 0) PASS("iasthreshold_below");
        else { FAIL("iasthreshold_below", vm.gp[7], 0); failures++; }
    }

    /* Test 9: IASCONVERGE */
    {
        FluxVM vm;
        flux_vm_init(&vm);
        /* R8=0, group base=(8/8)*8=8, so group is R8-R15 */
        vm.gp[9] = 100; vm.gp[10] = 200; vm.gp[11] = 300;
        vm.gp[12] = 400; vm.gp[13] = 500; vm.gp[14] = 600; vm.gp[15] = 700;
        vm.gp[8] = 0;
        uint8_t bc[] = {0xB6, 0x08}; /* IASCONVERGE R8 */
        flux_vm_execute(&vm, bc, 2);
        /* avg of gp[9-15] = (100+200+300+400+500+600+700)/7 = 2800/7 = 400 */
        /* R8 moves 25% toward 400: 0 + (400-0)/4 = 100 */
        if (vm.gp[8] == 100) PASS("iasconverge");
        else { printf("  FAIL: iasconverge (got %d, expected 100)\n", vm.gp[8]); failures++; }
    }

    /* Test 10: IASEXTINCT — prune (weak) */
    {
        FluxVM vm;
        flux_vm_init(&vm);
        vm.gp[9] = 5;
        uint8_t bc[] = {0xB7, 0x09}; /* IASEXTINCT R9 */
        flux_vm_execute(&vm, bc, 2);
        if (vm.gp[9] == 0) PASS("iasextinct_prune");
        else { FAIL("iasextinct_prune", vm.gp[9], 0); failures++; }
    }

    /* Test 11: IASEXTINCT — survive (strong) */
    {
        FluxVM vm;
        flux_vm_init(&vm);
        vm.gp[9] = 100;
        uint8_t bc[] = {0xB7, 0x09};
        flux_vm_execute(&vm, bc, 2);
        if (vm.gp[9] == 100) PASS("iasextinct_survive");
        else { FAIL("iasextinct_survive", vm.gp[9], 100); failures++; }
    }

    printf("\nInstinct opcode tests: %d failures\n", failures);
    return failures;
}
