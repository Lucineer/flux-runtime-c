#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "flux_vm.h"

#define PASS(name) printf("  PASS: %s\n", name)
#define FAIL(name) printf("  FAIL: %s\n", name)

int main(void) {
    int failures = 0;

    /* Test 1: INSTINCT_LOAD */
    {
        FluxVM vm;
        flux_vm_init(&vm);
        uint8_t bc[] = {0x68, 0x01, 0x00, 0x05}; /* INSTINCT_LOAD R1, 5 */
        vm.memory[5] = 42;
        flux_vm_run(&vm, bc, sizeof(bc));
        if (vm.gp[1] == 42) PASS("instinct_load");
        else { FAIL("instinct_load"); failures++; }
    }

    /* Test 2: INSTINCT_STORE */
    {
        FluxVM vm;
        flux_vm_init(&vm);
        vm.gp[2] = 99;
        uint8_t bc[] = {0x69, 0x02, 0x00, 0x0A}; /* INSTINCT_STORE R2, 10 */
        flux_vm_run(&vm, bc, sizeof(bc));
        if (vm.memory[10] == 99) PASS("instinct_store");
        else { FAIL("instinct_store"); failures++; }
    }

    /* Test 3: INSTINCT_DECAY */
    {
        FluxVM vm;
        flux_vm_init(&vm);
        vm.gp[3] = 1000;
        uint8_t bc[] = {0x6A, 0x03, 0x00, 0x00}; /* INSTINCT_DECAY R3 */
        flux_vm_run(&vm, bc, sizeof(bc));
        if (vm.gp[3] == 950) PASS("instinct_decay");
        else { FAIL("instinct_decay"); failures++; }
    }

    /* Test 4: INSTINCT_REFLEX (triggered) */
    {
        FluxVM vm;
        flux_vm_init(&vm);
        vm.gp[4] = 50; /* strong instinct */
        vm.gp[5] = 0;
        /* INSTINCT_REFLEX R4, addr6; then MOVI16 R5, 999; then target: MOVI16 R5, 777; HALT */
        uint8_t bc[] = {
            0x6B, 0x04, 0x00, 0x08, /* INSTINCT_REFLEX R4, 8 (jump to MOVI16 R5,777) */
            0x20, 0x05, 0x0F, 0xE7, /* MOVI16 R5, 999 (should be skipped) */
            0x20, 0x05, 0x03, 0x09, /* MOVI16 R5, 777 (target) */
            0x00,                   /* HALT */
        };
        flux_vm_run(&vm, bc, sizeof(bc));
        if (vm.gp[5] == 777 && vm.gp[4] == 49) PASS("instinct_reflex_trigger");
        else { FAIL("instinct_reflex_trigger"); failures++; }
    }

    /* Test 5: INSTINCT_REFLEX (not triggered) */
    {
        FluxVM vm;
        flux_vm_init(&vm);
        vm.gp[4] = 0; /* weak instinct */
        vm.gp[5] = 0;
        uint8_t bc[] = {
            0x6B, 0x04, 0x00, 0x08, /* INSTINCT_REFLEX R4, 8 */
            0x20, 0x05, 0x0F, 0xE7, /* MOVI16 R5, 999 */
            0x20, 0x05, 0x03, 0x09, /* MOVI16 R5, 777 */
            0x00,                   /* HALT */
        };
        flux_vm_run(&vm, bc, sizeof(bc));
        if (vm.gp[5] == 999) PASS("instinct_reflex_skip");
        else { FAIL("instinct_reflex_skip"); failures++; }
    }

    /* Test 6: INSTINCT_MODULATE */
    {
        FluxVM vm;
        flux_vm_init(&vm);
        vm.gp[6] = 500;
        uint8_t bc[] = {0x6C, 0x06, 0x00, 0x02}; /* INSTINCT_MODULATE R6, 2000/1000=2x */
        flux_vm_run(&vm, bc, sizeof(bc));
        if (vm.gp[6] == 1000) PASS("instinct_modulate");
        else { FAIL("instinct_modulate"); failures++; }
    }

    /* Test 7: INSTINCT_THRESHOLD (above) */
    {
        FluxVM vm;
        flux_vm_init(&vm);
        vm.gp[7] = 100;
        uint8_t bc[] = {0x6D, 0x07, 0x00, 0x50}; /* INSTINCT_THRESHOLD R7, 80 */
        flux_vm_run(&vm, bc, sizeof(bc));
        if (vm.gp[7] == 1) PASS("instinct_threshold_above");
        else { FAIL("instinct_threshold_above"); failures++; }
    }

    /* Test 8: INSTINCT_THRESHOLD (below) */
    {
        FluxVM vm;
        flux_vm_init(&vm);
        vm.gp[7] = 50;
        uint8_t bc[] = {0x6D, 0x07, 0x00, 0x64}; /* INSTINCT_THRESHOLD R7, 100 */
        flux_vm_run(&vm, bc, sizeof(bc));
        if (vm.gp[7] == 0) PASS("instinct_threshold_below");
        else { FAIL("instinct_threshold_below"); failures++; }
    }

    /* Test 9: INSTINCT_CONVERGE */
    {
        FluxVM vm;
        flux_vm_init(&vm);
        vm.gp[8] = 0;  vm.gp[9] = 100; vm.gp[10] = 200;
        vm.gp[11] = 300; vm.gp[12] = 400;
        uint8_t bc[] = {0x6E, 0x10, 0x00, 0x00}; /* INSTINCT_CONVERGE R16 */
        /* R16=0, group base=16, but we want to test with gp[8-12] */
        /* Let's use R8 which is in group 0 (0-7), avg of 1-7 = (0+0+...)/7 ~ 0 */
        uint8_t bc2[] = {0x6E, 0x08, 0x00, 0x00};
        vm.gp[0] = 100; vm.gp[1] = 200; vm.gp[2] = 300; vm.gp[3] = 400;
        vm.gp[4] = 500; vm.gp[5] = 600; vm.gp[6] = 700; vm.gp[7] = 800;
        vm.gp[8] = 0;
        flux_vm_run(&vm, bc2, sizeof(bc2));
        /* avg of gp[0-7] except gp[8] = (100+200+300+400+500+600+700+800)/8 = 4500/8 = 562 */
        /* gp[8] should move 25% toward 562: 0 + (562-0)/4 = 140 */
        if (vm.gp[8] == 140) PASS("instinct_converge");
        else { FAIL("instinct_converge"); failures++; printf("    got %d\n", vm.gp[8]); }
    }

    /* Test 10: INSTINCT_EXTINCT (prune) */
    {
        FluxVM vm;
        flux_vm_init(&vm);
        vm.gp[9] = 5; /* below extinction threshold of 10 */
        uint8_t bc[] = {0x6F, 0x09, 0x00, 0x00}; /* INSTINCT_EXTINCT R9 */
        flux_vm_run(&vm, bc, sizeof(bc));
        if (vm.gp[9] == 0) PASS("instinct_extinct_prune");
        else { FAIL("instinct_extinct_prune"); failures++; }
    }

    /* Test 11: INSTINCT_EXTINCT (survive) */
    {
        FluxVM vm;
        flux_vm_init(&vm);
        vm.gp[9] = 100; /* above extinction threshold */
        uint8_t bc[] = {0x6F, 0x09, 0x00, 0x00};
        flux_vm_run(&vm, bc, sizeof(bc));
        if (vm.gp[9] == 100) PASS("instinct_extinct_survive");
        else { FAIL("instinct_extinct_survive"); failures++; }
    }

    printf("\nInstinct opcode tests: %d failures\n", failures);
    return failures;
}
