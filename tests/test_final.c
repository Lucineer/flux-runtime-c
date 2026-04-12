#include <stdio.h>
#include <string.h>
#include "flux_vm.h"

#define PASS(name) printf("  PASS: %s\n", name)
#define FAIL(name, got, exp) printf("  FAIL: %s (got %d, exp %d)\n", name, got, exp)

int main(void) {
    int failures = 0;

    /* ── Trust Extended (0xC8-0xCF) ── */
    { FluxVM vm; flux_vm_init(&vm); vm.gp[1]=600; vm.gp[2]=800;
      uint8_t bc[] = {0xC8,0x01,0x02,0x00}; flux_vm_execute(&vm,bc,4);
      if (vm.gp[1]==700) PASS("trust_average"); else { FAIL("trust_average",vm.gp[1],700); failures++; } }

    { FluxVM vm; flux_vm_init(&vm); vm.gp[1]=500; vm.gp[2]=600;
      uint8_t bc[] = {0xC9,0x01,0x02,0x00}; flux_vm_execute(&vm,bc,4);
      if (vm.gp[1]==1000) PASS("trust_boost_cap"); else { FAIL("trust_boost_cap",vm.gp[1],1000); failures++; } }

    { FluxVM vm; flux_vm_init(&vm); vm.gp[1]=300; vm.gp[2]=100;
      uint8_t bc[] = {0xCA,0x01,0x02,0x00}; flux_vm_execute(&vm,bc,4);
      if (vm.gp[1]==200) PASS("trust_weaken"); else { FAIL("trust_weaken",vm.gp[1],200); failures++; } }

    { FluxVM vm; flux_vm_init(&vm); vm.gp[1]=0;
      uint8_t bc[] = {0xCD,0x01,0x64,0x00}; /* seed 0-100 */
      flux_vm_execute(&vm,bc,4);
      if (vm.gp[1]>=0 && vm.gp[1]<100) PASS("trust_seed"); else { FAIL("trust_seed",vm.gp[1],-1); failures++; } }

    /* ── Memory Extended (0xD8-0xDF) ── */
    { FluxVM vm; flux_vm_init(&vm); vm.gp[1]=42; vm.gp[2]=100;
      vm.memory[100]=0; vm.memory[102]=42; vm.memory[103]=0;
      uint8_t bc[] = {0xD8,0x01,0x02,0x04,0x00}; /* scan 4 bytes at mem[R2] */
      flux_vm_execute(&vm,bc,5);
      if (vm.zero_flag==1) PASS("memscan_found"); else { FAIL("memscan_found",vm.zero_flag,1); failures++; } }

    { FluxVM vm; flux_vm_init(&vm); vm.gp[1]=99; vm.gp[2]=100;
      vm.memory[100]=0; vm.memory[101]=0;
      uint8_t bc[] = {0xD8,0x01,0x02,0x02,0x00};
      flux_vm_execute(&vm,bc,5);
      if (vm.zero_flag==0) PASS("memscan_notfound"); else { FAIL("memscan_notfound",vm.zero_flag,0); failures++; } }

    { FluxVM vm; flux_vm_init(&vm); vm.gp[1]=0; vm.gp[2]=100;
      vm.memory[100]=5; vm.memory[101]=10; vm.memory[102]=15; vm.memory[103]=0;
      uint8_t bc[] = {0xDA,0x01,0x02,0x03,0x00}; /* sum 3 bytes */
      flux_vm_execute(&vm,bc,5);
      if (vm.gp[1]==30) PASS("memsum"); else { FAIL("memsum",vm.gp[1],30); failures++; } }

    { FluxVM vm; flux_vm_init(&vm); vm.gp[1]=0; vm.gp[2]=100;
      vm.memory[100]=200; vm.memory[101]=50; vm.memory[102]=150;
      uint8_t bc[] = {0xDB,0x01,0x02,0x03,0x00}; /* range min */
      flux_vm_execute(&vm,bc,5);
      if (vm.gp[1]==50) PASS("memrange_min"); else { FAIL("memrange_min",vm.gp[1],50); failures++; } }

    { FluxVM vm; flux_vm_init(&vm); vm.gp[1]=0; vm.gp[2]=100;
      vm.memory[100]=50; vm.memory[101]=200; vm.memory[102]=100;
      uint8_t bc[] = {0xDC,0x01,0x02,0x03,0x00}; /* range max */
      flux_vm_execute(&vm,bc,5);
      if (vm.gp[1]==200) PASS("memrange_max"); else { FAIL("memrange_max",vm.gp[1],200); failures++; } }

    { FluxVM vm; flux_vm_init(&vm);
      uint8_t bc[] = {0xDE,0x01,0x10,0x00}; /* alloc 16 bytes */
      flux_vm_execute(&vm,bc,4);
      if (vm.gp[1]==32768 && !vm.faulted) PASS("heap_alloc"); else { FAIL("heap_alloc",vm.gp[1],32768); failures++; } }

    { FluxVM vm; flux_vm_init(&vm);
      uint8_t bc[] = {0xDF,0x01}; flux_vm_execute(&vm,bc,2);
      PASS("heap_free"); } /* just checks it doesn't crash */

    /* ── Time & Random (0xE8-0xEF) ── */
    { FluxVM vm; flux_vm_init(&vm); vm.gp[1]=0;
      uint8_t bc[] = {0xEA,0x01}; /* read cycles */
      flux_vm_execute(&vm,bc,2);
      if (vm.gp[1]>=1) PASS("cycle_read"); else { FAIL("cycle_read",vm.gp[1],1); failures++; } }

    { FluxVM vm; flux_vm_init(&vm); vm.gp[1]=0; vm.gp[2]=10;
      uint8_t bc[] = {0xE8,0x01,0x02,0x00}; /* rand in [0,10) */
      flux_vm_execute(&vm,bc,4);
      if (vm.gp[1]>=0 && vm.gp[1]<10) PASS("rand_range"); else { FAIL("rand_range",vm.gp[1],-1); failures++; } }

    /* VARINT roundtrip: encode then decode */
    { FluxVM vm; flux_vm_init(&vm); vm.gp[1]=300;
      uint8_t bc[] = {
        0xED, 0x01, 0x00, 0x64, /* encode R1(300) to mem[100], R1=bytes written */
        0x00                     /* HALT */
      };
      flux_vm_execute(&vm, bc, 4);
      int bytes_written = vm.gp[1];
      /* Now decode from same address */
      vm.gp[3] = 0; /* will hold result */
      uint8_t bc2[] = {0xEE, 0x03, 0x00, 0x64, 0x00};
      flux_vm_execute(&vm, bc2, 4);
      if (vm.gp[3]==300) PASS("varint_roundtrip"); else { FAIL("varint_roundtrip",vm.gp[3],300); failures++; } }

    /* CRC32 */
    { FluxVM vm; flux_vm_init(&vm); vm.gp[1]=0; vm.gp[2]=100;
      vm.memory[100]='I'; vm.memory[101]='E'; vm.memory[102]='T'; vm.memory[103]='F';
      uint8_t bc[] = {0xEC,0x01,0x02,0x04,0x00}; /* CRC32 of 4 bytes */
      flux_vm_execute(&vm,bc,5);
      if (vm.gp[1]!=0) PASS("crc32"); else { FAIL("crc32",vm.gp[1],-1); failures++; } }

    /* ATOMIC_CAS success */
    { FluxVM vm; flux_vm_init(&vm); vm.gp[1]=100; vm.gp[2]=42; vm.gp[3]=99;
      vm.memory[100]=42;
      uint8_t bc[] = {0xEF,0x01,0x02,0x03}; /* CAS mem[R1]: if 42==R2, set to R3 */
      flux_vm_execute(&vm,bc,4);
      if (vm.zero_flag==1 && vm.memory[100]==99) PASS("cas_success"); else { FAIL("cas_success",vm.zero_flag,1); failures++; } }

    /* ATOMIC_CAS failure */
    { FluxVM vm; flux_vm_init(&vm); vm.gp[1]=100; vm.gp[2]=42; vm.gp[3]=99;
      vm.memory[100]=50; /* mismatch */
      uint8_t bc[] = {0xEF,0x01,0x02,0x03};
      flux_vm_execute(&vm,bc,4);
      if (vm.zero_flag==0 && vm.memory[100]==50) PASS("cas_fail"); else { FAIL("cas_fail",vm.zero_flag,0); failures++; } }

    printf("\nFinal opcode tests: %d failures\n", failures);
    return failures;
}
