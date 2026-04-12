#include <stdio.h>
#include <string.h>
#include "flux_vm.h"

#define PASS(name) printf("  PASS: %s\n", name)
#define FAIL(name, got, exp) printf("  FAIL: %s (got %d, exp %d)\n", name, got, exp)

int main(void) {
    int failures = 0;

    /* ── Instinct Extended (0xB8-0xBF) ── */

    /* HABITUATE: strengthen instinct */
    { FluxVM vm; flux_vm_init(&vm); vm.gp[1] = 100;
      uint8_t bc[] = {0xB8, 0x01, 0x00}; flux_vm_execute(&vm, bc, 2);
      if (vm.gp[1] == 101) PASS("habituate"); else { FAIL("habituate", vm.gp[1], 101); failures++; } }

    /* SENSITIZE: double (up to 255) */
    { FluxVM vm; flux_vm_init(&vm); vm.gp[1] = 50;
      uint8_t bc[] = {0xB9, 0x01, 0x00}; flux_vm_execute(&vm, bc, 2);
      if (vm.gp[1] == 100) PASS("sensitize"); else { FAIL("sensitize", vm.gp[1], 100); failures++; } }

    /* GENERALIZE: average with neighbor */
    { FluxVM vm; flux_vm_init(&vm); vm.gp[1] = 100; vm.gp[2] = 200;
      uint8_t bc[] = {0xBA, 0x01, 0x02, 0x00}; flux_vm_execute(&vm, bc, 4);
      if (vm.gp[1] == 150) PASS("generalize"); else { FAIL("generalize", vm.gp[1], 150); failures++; } }

    /* INHIBIT: suppress rs1 when rd active */
    { FluxVM vm; flux_vm_init(&vm); vm.gp[1] = 50; vm.gp[2] = 100;
      uint8_t bc[] = {0xBD, 0x01, 0x02, 0x00}; flux_vm_execute(&vm, bc, 4);
      if (vm.gp[2] == 0 && vm.gp[1] == 50) PASS("inhibit"); else { FAIL("inhibit", vm.gp[2], 0); failures++; } }

    /* INHIBIT: no suppress when rd inactive */
    { FluxVM vm; flux_vm_init(&vm); vm.gp[1] = 0; vm.gp[2] = 100;
      uint8_t bc[] = {0xBD, 0x01, 0x02, 0x00}; flux_vm_execute(&vm, bc, 4);
      if (vm.gp[2] == 100) PASS("inhibit_nop"); else { FAIL("inhibit_nop", vm.gp[2], 100); failures++; } }

    /* SUM: rd += rs1 */
    { FluxVM vm; flux_vm_init(&vm); vm.gp[1] = 30; vm.gp[2] = 70;
      uint8_t bc[] = {0xBE, 0x01, 0x02, 0x00}; flux_vm_execute(&vm, bc, 4);
      if (vm.gp[1] == 100) PASS("instinct_sum"); else { FAIL("instinct_sum", vm.gp[1], 100); failures++; } }

    /* DIFF: rd -= rs1 */
    { FluxVM vm; flux_vm_init(&vm); vm.gp[1] = 100; vm.gp[2] = 30;
      uint8_t bc[] = {0xBF, 0x01, 0x02, 0x00}; flux_vm_execute(&vm, bc, 4);
      if (vm.gp[1] == 70) PASS("instinct_diff"); else { FAIL("instinct_diff", vm.gp[1], 70); failures++; } }

    /* ── Trust (0xC0-0xCF) ── */

    /* TRUST_INIT: set base trust */
    { FluxVM vm; flux_vm_init(&vm);
      uint8_t bc[] = {0xC0, 0x01, 0x58, 0x02}; /* imm16=600 */
      flux_vm_execute(&vm, bc, 4);
      if (vm.gp[1] == 600) PASS("trust_init"); else { FAIL("trust_init", vm.gp[1], 600); failures++; } }

    /* TRUST_UPDATE: bayesian nudge */
    { FluxVM vm; flux_vm_init(&vm); vm.gp[1] = 500; vm.gp[2] = 1000;
      uint8_t bc[] = {0xC1, 0x01, 0x02, 0x00}; flux_vm_execute(&vm, bc, 4);
      /* 500 + (1000-500)/10 = 500 + 50 = 550 */
      if (vm.gp[1] == 550) PASS("trust_update"); else { FAIL("trust_update", vm.gp[1], 550); failures++; } }

    /* TRUST_DECAY: 99% */
    { FluxVM vm; flux_vm_init(&vm); vm.gp[1] = 1000;
      uint8_t bc[] = {0xC2, 0x01}; flux_vm_execute(&vm, bc, 2);
      if (vm.gp[1] == 990) PASS("trust_decay"); else { FAIL("trust_decay", vm.gp[1], 990); failures++; } }

    /* TRUST_COMPARE: set zero flag */
    { FluxVM vm; flux_vm_init(&vm); vm.gp[1] = 800; vm.gp[2] = 500;
      uint8_t bc[] = {0xC3, 0x01, 0x02, 0x00}; flux_vm_execute(&vm, bc, 4);
      if (vm.zero_flag == 1) PASS("trust_compare_ge"); else { FAIL("trust_compare_ge", vm.zero_flag, 1); failures++; } }

    /* TRUST_REVOKE: set to 0 */
    { FluxVM vm; flux_vm_init(&vm); vm.gp[1] = 999;
      uint8_t bc[] = {0xC5, 0x01}; flux_vm_execute(&vm, bc, 2);
      if (vm.gp[1] == 0) PASS("trust_revoke"); else { FAIL("trust_revoke", vm.gp[1], 0); failures++; } }

    /* TRUST_RESTRICT: cap trust */
    { FluxVM vm; flux_vm_init(&vm); vm.gp[1] = 900;
      uint8_t bc[] = {0xC6, 0x01, 0xE8, 0x03}; /* imm16=1000, no cap */
      flux_vm_execute(&vm, bc, 4);
      if (vm.gp[1] == 900) PASS("trust_restrict_nocap"); else { FAIL("trust_restrict_nocap", vm.gp[1], 900); failures++; } }

    /* TRUST_SCORE: weighted average */
    { FluxVM vm; flux_vm_init(&vm); vm.gp[2] = 600; vm.gp[3] = 800;
      uint8_t bc[] = {0xC7, 0x01, 0x02, 0x03}; /* R1 = R2*3 + R3*7 / 10 */
      flux_vm_execute(&vm, bc, 4);
      if (vm.gp[1] == 740) PASS("trust_score"); else { FAIL("trust_score", vm.gp[1], 740); failures++; } }

    /* ── Memory Management (0xD0-0xDF) ── */

    /* MEMSET: fill range */
    { FluxVM vm; flux_vm_init(&vm); vm.gp[1] = 0x42;
      uint8_t bc[] = {0xD0, 0x01, 0x0A, 0x00, 0x05, 0x00}; /* fill 5 bytes at addr 10 */
      flux_vm_execute(&vm, bc, 5);
      if (vm.memory[10]==0x42 && vm.memory[14]==0x42 && vm.memory[15]==0) PASS("memset");
      else { FAIL("memset", vm.memory[10], 0x42); failures++; } }

    /* MEMCPY: copy range */
    { FluxVM vm; flux_vm_init(&vm);
      vm.memory[0]=1; vm.memory[1]=2; vm.memory[2]=3;
      uint8_t bc[] = {0xD1, 0x14, 0x00, 0x00, 0x03, 0x00}; /* copy 3 bytes from addr 0 to addr 20 */
      flux_vm_execute(&vm, bc, 5);
      if (vm.memory[20]==1 && vm.memory[22]==3) PASS("memcpy"); else { FAIL("memcpy", vm.memory[20], 1); failures++; } }

    /* MEMCMP: equal */
    { FluxVM vm; flux_vm_init(&vm);
      vm.memory[0]=1; vm.memory[1]=2; vm.memory[10]=1; vm.memory[11]=2;
      uint8_t bc[] = {0xD2, 0x00, 0x0A, 0x00, 0x02, 0x00}; /* compare 2 bytes */
      flux_vm_execute(&vm, bc, 5);
      if (vm.zero_flag == 1) PASS("memcmp_eq"); else { FAIL("memcmp_eq", vm.zero_flag, 1); failures++; } }

    /* MEMCMP: not equal */
    { FluxVM vm; flux_vm_init(&vm);
      vm.memory[0]=1; vm.memory[10]=2;
      uint8_t bc[] = {0xD2, 0x00, 0x0A, 0x00, 0x01, 0x00};
      flux_vm_execute(&vm, bc, 5);
      if (vm.zero_flag == 0) PASS("memcmp_ne"); else { FAIL("memcmp_ne", vm.zero_flag, 0); failures++; } }

    /* MEMSWAP: swap register values */
    { FluxVM vm; flux_vm_init(&vm); vm.gp[1] = 42; vm.gp[2] = 99;
      uint8_t bc[] = {0xD4, 0x01, 0x02, 0x00};
      flux_vm_execute(&vm, bc, 4);
      if (vm.gp[1]==99 && vm.gp[2]==42) PASS("memswap"); else { FAIL("memswap", vm.gp[1], 99); failures++; } }

    /* ── Bit Manipulation (0xE0-0xEF) ── */

    /* BITCOUNT: popcount */
    { FluxVM vm; flux_vm_init(&vm); vm.gp[1] = 0xFF; /* 8 bits set */
      uint8_t bc[] = {0xE0, 0x01}; flux_vm_execute(&vm, bc, 2);
      if (vm.gp[1] == 8) PASS("bitcount"); else { FAIL("bitcount", vm.gp[1], 8); failures++; } }

    /* BITCOUNT: zero */
    { FluxVM vm; flux_vm_init(&vm); vm.gp[1] = 0;
      uint8_t bc[] = {0xE0, 0x01}; flux_vm_execute(&vm, bc, 2);
      if (vm.gp[1] == 0) PASS("bitcount_zero"); else { FAIL("bitcount_zero", vm.gp[1], 0); failures++; } }

    /* BITSCAN: CLZ */
    { FluxVM vm; flux_vm_init(&vm); vm.gp[1] = 1; /* 0x00000001, CLZ=31 */
      uint8_t bc[] = {0xE1, 0x01}; flux_vm_execute(&vm, bc, 2);
      if (vm.gp[1] == 31) PASS("bitscan"); else { FAIL("bitscan", vm.gp[1], 31); failures++; } }

    /* BITSCAN: zero */
    { FluxVM vm; flux_vm_init(&vm); vm.gp[1] = 0;
      uint8_t bc[] = {0xE1, 0x01}; flux_vm_execute(&vm, bc, 2);
      if (vm.gp[1] == 32) PASS("bitscan_zero"); else { FAIL("bitscan_zero", vm.gp[1], 32); failures++; } }

    /* BITREV: bit reverse of 1 = 0x80000000 */
    { FluxVM vm; flux_vm_init(&vm); vm.gp[1] = 1;
      uint8_t bc[] = {0xE2, 0x01}; flux_vm_execute(&vm, bc, 2);
      if (vm.gp[1] == (int32_t)0x80000000) PASS("bitrev"); else { printf("  FAIL: bitrev (got 0x%08X)\n", vm.gp[1]); failures++; } }

    /* ROTL: rotate left by 4 */
    { FluxVM vm; flux_vm_init(&vm); vm.gp[1] = 1; vm.gp[2] = 4; /* 1 << 4 = 16 */
      uint8_t bc[] = {0xE3, 0x01, 0x02, 0x00}; flux_vm_execute(&vm, bc, 4);
      if (vm.gp[1] == 16) PASS("rotl"); else { FAIL("rotl", vm.gp[1], 16); failures++; } }

    /* ROTR: rotate right by 4 */
    { FluxVM vm; flux_vm_init(&vm); vm.gp[1] = 16; vm.gp[2] = 4; /* 16 >> 4 = 1 */
      uint8_t bc[] = {0xE4, 0x01, 0x02, 0x00}; flux_vm_execute(&vm, bc, 4);
      if (vm.gp[1] == 1) PASS("rotr"); else { FAIL("rotr", vm.gp[1], 1); failures++; } }

    printf("\nExtended opcode tests: %d failures\n", failures);
    return failures;
}
