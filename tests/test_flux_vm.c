/* test_flux_vm.c — Extended FLUX VM tests
 * Oracle1's 19 tests + JetsonClaw1's additions for LOAD/STORE, A2A, CONF, sensors
 */
#include "flux_vm.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static int passed = 0, failed = 0;
#define TEST(name) printf("  %-50s", #name);
#define PASS() do { printf("PASS\n"); passed++; } while(0)
#define FAIL(msg) do { printf("FAIL: %s\n", msg); failed++; } while(0)
#define EQ(a,b) do { if ((a)!=(b)) { FAIL(#a " != " #b); return; } } while(0)

/* ═══ Oracle1's original 19 tests ═══ */

void test_init() {
    TEST(init); FluxVM vm; flux_vm_init(&vm);
    EQ(vm.halted, false); EQ(vm.pc, 0); EQ(vm.cycles, 0); EQ(vm.sp, FLUX_STACK_SIZE); PASS();
}

void test_halt() {
    TEST(halt); FluxVM vm; flux_vm_init(&vm);
    uint8_t bc[] = { OP_HALT }; flux_vm_execute(&vm, bc, sizeof(bc));
    EQ(vm.halted, true); EQ(vm.cycles, 1); PASS();
}

void test_nop() {
    TEST(nop); FluxVM vm; flux_vm_init(&vm);
    uint8_t bc[] = { OP_NOP, OP_HALT }; flux_vm_execute(&vm, bc, sizeof(bc));
    EQ(vm.halted, true); EQ(vm.cycles, 2); PASS();
}

void test_movi() {
    TEST(movi_imm8); FluxVM vm; flux_vm_init(&vm);
    uint8_t bc[] = { OP_MOVI, 1, 42, OP_HALT }; flux_vm_execute(&vm, bc, sizeof(bc));
    EQ(vm.gp[1], 42); PASS();
}

void test_movi_negative() {
    TEST(movi_negative); FluxVM vm; flux_vm_init(&vm);
    uint8_t bc[] = { OP_MOVI, 1, (uint8_t)(-5), OP_HALT }; flux_vm_execute(&vm, bc, sizeof(bc));
    EQ(vm.gp[1], -5); PASS();
}

void test_movi16() {
    TEST(movi16); FluxVM vm; flux_vm_init(&vm);
    uint8_t bc[] = { OP_MOVI16, 1, 0xE8, 0x03, OP_HALT }; flux_vm_execute(&vm, bc, sizeof(bc));
    EQ(vm.gp[1], 1000); PASS();
}

void test_movi16_negative() {
    TEST(movi16_negative); FluxVM vm; flux_vm_init(&vm);
    uint8_t bc[] = { OP_MOVI16, 1, 0x00, 0x80, OP_HALT }; flux_vm_execute(&vm, bc, sizeof(bc));
    EQ(vm.gp[1], -32768); PASS();
}

void test_add() {
    TEST(add); FluxVM vm; flux_vm_init(&vm);
    uint8_t bc[] = { OP_MOVI,1,10, OP_MOVI,2,20, OP_ADD,3,1,2, OP_HALT };
    flux_vm_execute(&vm, bc, sizeof(bc)); EQ(vm.gp[3], 30); PASS();
}

void test_sub() {
    TEST(sub); FluxVM vm; flux_vm_init(&vm);
    uint8_t bc[] = { OP_MOVI,1,50, OP_MOVI,2,20, OP_SUB,3,1,2, OP_HALT };
    flux_vm_execute(&vm, bc, sizeof(bc)); EQ(vm.gp[3], 30); PASS();
}

void test_mul() {
    TEST(mul); FluxVM vm; flux_vm_init(&vm);
    uint8_t bc[] = { OP_MOVI,1,7, OP_MOVI,2,6, OP_MUL,3,1,2, OP_HALT };
    flux_vm_execute(&vm, bc, sizeof(bc)); EQ(vm.gp[3], 42); PASS();
}

void test_div() {
    TEST(div); FluxVM vm; flux_vm_init(&vm);
    uint8_t bc[] = { OP_MOVI,1,42, OP_MOVI,2,7, OP_DIV,3,1,2, OP_HALT };
    flux_vm_execute(&vm, bc, sizeof(bc)); EQ(vm.gp[3], 6); PASS();
}

void test_inc_dec() {
    TEST(inc_dec); FluxVM vm; flux_vm_init(&vm);
    uint8_t bc[] = { OP_MOVI,1,10, OP_INC,1, OP_INC,1, OP_DEC,1, OP_HALT };
    flux_vm_execute(&vm, bc, sizeof(bc)); EQ(vm.gp[1], 11); PASS();
}

void test_push_pop() {
    TEST(push_pop); FluxVM vm; flux_vm_init(&vm);
    uint8_t bc[] = { OP_MOVI,1,42, OP_PUSH,1, OP_MOVI,1,0, OP_POP,2, OP_HALT };
    flux_vm_execute(&vm, bc, sizeof(bc)); EQ(vm.gp[2], 42); PASS();
}

void test_jnz_loop() {
    TEST(jnz_loop_count_to_5); FluxVM vm; flux_vm_init(&vm);
    /* Use MOVI16 to load offset into R3, then JNZ R1, R3 */
    uint8_t bc[] = {
        OP_MOVI, 1, 5,                /* R1 = 5 (counter) */
        OP_MOVI, 2, 0,                /* R2 = 0 (result) */
        /* loop: */
        OP_INC, 2,                    /* R2++ */
        OP_DEC, 1,                    /* R1-- */
        OP_MOVI16, 3, 0xF8, 0xFF,     /* R3 = -8 (jump back 8 bytes to INC) */
        OP_JNZ, 1, 3, 0,              /* JNZ R1, R3 (= -4) */
        OP_HALT
    };
    flux_vm_execute(&vm, bc, sizeof(bc));
    EQ(vm.gp[1], 0); EQ(vm.gp[2], 5); PASS();
}

void test_loop_instruction() {
    TEST(loop_instruction); FluxVM vm; flux_vm_init(&vm);
    uint8_t bc[] = {
        OP_MOVI, 1, 5,                /* 0-2: R1=5 */
        OP_MOVI, 2, 0,                /* 3-5: R2=0 */
        /* loop @ byte 6: */
        OP_ADD, 2, 2, 1,              /* 6-9: R2 += R1 */
        OP_LOOP, 1, 0xFC, 0xFF,       /* 10-13: LOOP R1, -4 (back to ADD) */
        OP_HALT
    };
    flux_vm_execute(&vm, bc, sizeof(bc));
    /* R1: 5→4→3→2→1→0. R2: 5+4+3+2+1 = 15 */
    EQ(vm.gp[2], 15); PASS();
}

void test_addi_subi() {
    TEST(addi_subi); FluxVM vm; flux_vm_init(&vm);
    uint8_t bc[] = { OP_MOVI,1,10, OP_ADDI,1,5, OP_SUBI,1,3, OP_HALT };
    flux_vm_execute(&vm, bc, sizeof(bc)); EQ(vm.gp[1], 12); PASS();
}

void test_cmp_eq() {
    TEST(cmp_eq); FluxVM vm; flux_vm_init(&vm);
    uint8_t bc[] = { OP_MOVI,1,5, OP_MOVI,2,5, OP_CMP_EQ,3,1,2, OP_HALT };
    flux_vm_execute(&vm, bc, sizeof(bc)); EQ(vm.gp[3], 1); PASS();
}

void test_stripconf() {
    TEST(stripconf); FluxVM vm; flux_vm_init(&vm);
    uint8_t bc[] = { OP_STRIPCF, 3, OP_MOVI,1,42, OP_MOVI,2,10, OP_ADD,3,1,2, OP_HALT };
    flux_vm_execute(&vm, bc, sizeof(bc));
    EQ(vm.stripconf_remaining, 0); EQ(vm.gp[3], 52); PASS();
}

void test_fibonacci() {
    TEST(fibonacci); FluxVM vm; flux_vm_init(&vm);
    /* fib: a=1,b=1, iterate 10 times */
    uint8_t bc[] = {
        OP_MOVI, 1, 1,                /* R1 = a = 1 */
        OP_MOVI, 2, 1,                /* R2 = b = 1 */
        OP_MOVI, 3, 10,               /* R3 = count = 10 */
        /* loop: */
        OP_ADD, 4, 1, 2,              /* R4 = a + b */
        OP_MOV, 1, 2, 0,              /* a = b */
        OP_MOV, 2, 4, 0,              /* b = R4 */
        OP_DEC, 3,                    /* count-- */
        OP_MOVI16, 5, 0xEE, 0xFF,     /* R5 = -18 */
        OP_JNZ, 3, 5, 0,              /* JNZ count, -16 */
        OP_HALT
    };
    flux_vm_execute(&vm, bc, sizeof(bc));
    EQ(vm.gp[2], 144); PASS(); /* fib sequence: 1,1,2,3,5,8,13,21,34,55,89,144 */
}

/* ═══ JetsonClaw1 extensions ═══ */

void test_load_store() {
    TEST(load_store); FluxVM vm; flux_vm_init(&vm);
    uint8_t bc[] = {
        OP_MOVI16, 1, 0x00, 0x00,     /* R1 = 0 (base addr) */
        OP_MOVI16, 2, 0x10, 0x27,     /* R2 = 10000 */
        OP_MOVI, 3, 0,                 /* R3 = 0 (offset) */
        OP_STORE, 2, 1, 3,             /* mem[R1+R3] = R2 */
        OP_MOV, 3, 0, 0,               /* R3 = 0 (reset offset) */
        OP_LOAD, 4, 1, 3,              /* R4 = mem[R1+R3] */
        OP_HALT
    };
    flux_vm_execute(&vm, bc, sizeof(bc));
    EQ(vm.gp[4], 10000); PASS();
}

void test_load_store_byte() {
    TEST(load_store_byte); FluxVM vm; flux_vm_init(&vm);
    uint8_t bc[] = {
        OP_MOVI, 1, 42,
        OP_STI8, 1, 0x10, 0x00,       /* mem[0x0010] = R1 */
        OP_MOV, 1, 0, 0,               /* R1 = 0 (reset) */
        OP_LDI8, 2, 0x10, 0x00,       /* R2 = mem[0x0010] */
        OP_HALT
    };
    flux_vm_execute(&vm, bc, sizeof(bc));
    EQ(vm.gp[2], 42); PASS();
}

void test_loadi_storei() {
    TEST(loadi_storei_format_g); FluxVM vm; flux_vm_init(&vm);
    /* LOADI R2, R1, imm16: R2 = mem[R1+imm16] */
    uint8_t bc[] = {
        OP_MOVI16, 1, 0x00, 0x00,     /* R1 = 0 */
        OP_MOVI, 2, 99,
        OP_STOREI, 2, 1, 0x20, 0x00,   /* mem[0+32] = R2 */
        OP_MOV, 2, 0, 0,               /* R2 = 0 (reset) */
        OP_LOADI, 3, 1, 0x20, 0x00,    /* R3 = mem[0+32] */
        OP_HALT
    };
    flux_vm_execute(&vm, bc, sizeof(bc));
    EQ(vm.gp[3], 99); PASS();
}

void test_conf_set_get() {
    TEST(conf_set_get); FluxVM vm; flux_vm_init(&vm);
    uint8_t bc[] = {
        OP_MOVI, 1, 75,                /* R1 = 75 */
        OP_CONF_SET, 2, 1, 0,          /* conf[2] = R1 = 75 */
        OP_CONF_GET, 3, 2, 0,          /* R3 = conf[2] */
        OP_HALT
    };
    flux_vm_execute(&vm, bc, sizeof(bc));
    EQ(vm.conf[2], 75); EQ(vm.gp[3], 75); PASS();
}

void test_conf_add_harmonic() {
    TEST(conf_add_harmonic_mean); FluxVM vm; flux_vm_init(&vm);
    uint8_t bc[] = {
        OP_MOVI, 1, 50,                /* R1 = 50 */
        OP_CONF_SET, 1, 1, 0,          /* conf[1] = 50 */
        OP_MOVI, 2, 50,                /* R2 = 50 */
        OP_CONF_SET, 2, 2, 0,          /* conf[2] = 50 */
        OP_CONF_ADD, 3, 1, 2,          /* conf[3] = harmonic_mean(conf[1], conf[2]) */
        OP_HALT
    };
    flux_vm_execute(&vm, bc, sizeof(bc));
    /* harmonic_mean(50,50) = 2*50*50/100 = 50 */
    EQ(vm.conf[3], 50); PASS();
}

void test_conf_fuse_bayesian() {
    TEST(conf_fuse_bayesian); FluxVM vm; flux_vm_init(&vm);
    uint8_t bc[] = {
        OP_MOVI16, 1, 0x58, 0x02,      /* R1 = 600 */
        OP_CONF_SET, 1, 1, 0,           /* conf[1] = 600 (but clamped concept) */
        OP_MOVI16, 2, 0x2C, 0x01,      /* R2 = 300 */
        OP_CONF_SET, 2, 2, 0,           /* conf[2] = 300 */
        OP_MOVI16, 1, 0x0A, 0x00,      /* R1 = 10 */
        OP_MOVI16, 2, 0x14, 0x00,      /* R2 = 20 */
        OP_CONF_FUSE, 3, 1, 2,          /* conf[3] = (10*600+20*300)/900 = 13 */
        OP_HALT
    };
    flux_vm_execute(&vm, bc, sizeof(bc));
    EQ(vm.conf[3], 13); PASS();
}

void test_conf_clamp() {
    TEST(conf_clamp); FluxVM vm; flux_vm_init(&vm);
    uint8_t bc[] = {
        OP_MOVI16, 1, 0xFF, 0x7F,      /* R1 = 32767 */
        OP_CONF_SET, 1, 1, 0,           /* conf[1] = 32767 */
        OP_CONF_CLAMP, 1, 0, 0,         /* conf[1] = clamp(conf[1], 0, 100) */
        OP_CONF_GET, 2, 1, 0,
        OP_HALT
    };
    flux_vm_execute(&vm, bc, sizeof(bc));
    EQ(vm.conf[1], 100); PASS();
}

void test_a2a_tell_stub() {
    TEST(a2a_tell_stub); FluxVM vm; flux_vm_init(&vm);
    uint8_t bc[] = { OP_TELL, 1, 2, 3, OP_HALT };
    flux_vm_execute(&vm, bc, sizeof(bc));
    EQ(vm.halted, true); EQ(vm.cycles, 2); PASS(); /* TELL is a no-op stub */
}

void test_a2a_fork() {
    TEST(a2a_fork); FluxVM vm; flux_vm_init(&vm);
    vm.agent_id = 42;
    uint8_t bc[] = { OP_FORK, 1, 2, 3, OP_HALT };
    flux_vm_execute(&vm, bc, sizeof(bc));
    EQ(vm.gp[1], 42); EQ(vm.cycles, 12); PASS(); /* fork: 1(cycle++) + 10(penalty) + 1(halt) = 12 */
}

void test_sensor_stubs() {
    TEST(sensor_stubs); FluxVM vm; flux_vm_init(&vm);
    uint8_t bc[] = { OP_SENSE, 1, 0, 0, OP_GPS, 2, 0, 0, OP_ACCEL, 3, 0, 0, OP_TEMP, 4, 0, 0, OP_HALT };
    flux_vm_execute(&vm, bc, sizeof(bc));
    EQ(vm.gp[1], 0); EQ(vm.gp[2], 0); EQ(vm.gp[3], 0); EQ(vm.gp[4], 0); PASS();
}

void test_atp_gen_use() {
    TEST(atp_gen_use); FluxVM vm; flux_vm_init(&vm);
    uint8_t bc[] = {
        OP_ATP_GEN, 1, 0, 0,            /* R1 = 100 ATP */
        OP_MOVI, 2, 30,
        OP_ATP_USE, 1, 2, 0,            /* R1 -= R2 */
        OP_HALT
    };
    flux_vm_execute(&vm, bc, sizeof(bc));
    EQ(vm.gp[1], 70); PASS();
}

void test_apoptosis() {
    TEST(apoptosis); FluxVM vm; flux_vm_init(&vm);
    uint8_t bc[] = { OP_APOPTOSIS, 0, 0, 0 };
    flux_vm_execute(&vm, bc, sizeof(bc));
    EQ(vm.halted, true); EQ(vm.fault_code, 99); PASS();
}

void test_abs_sqrt() {
    TEST(abs_sqrt); FluxVM vm; flux_vm_init(&vm);
    uint8_t bc[] = {
        OP_MOVI16, 1, 0xF6, 0xFF,      /* R1 = -10 */
        OP_ABS, 2, 1, 0,                /* R2 = abs(-10) = 10 */
        OP_SQRT, 3, 2, 0,               /* R3 = sqrt(10) = 3 */
        OP_HALT
    };
    flux_vm_execute(&vm, bc, sizeof(bc));
    EQ(vm.gp[2], 10); EQ(vm.gp[3], 3); PASS();
}

void test_rand() {
    TEST(rand); FluxVM vm; flux_vm_init(&vm);
    uint8_t bc[] = { OP_RAND, 1, 0, 0, OP_HALT };
    flux_vm_execute(&vm, bc, sizeof(bc));
    /* R1 should be some non-zero value (probabilistic) */
    EQ(vm.halted, true); PASS();
}

void test_swap() {
    TEST(swap); FluxVM vm; flux_vm_init(&vm);
    uint8_t bc[] = { OP_MOVI,1,10, OP_MOVI,2,20, OP_SWP,1,2, OP_HALT };
    flux_vm_execute(&vm, bc, sizeof(bc));
    EQ(vm.gp[1], 20); EQ(vm.gp[2], 10); PASS();
}

void test_r0_always_zero() {
    TEST(r0_reads_zero); FluxVM vm; flux_vm_init(&vm);
    uint8_t bc[] = { OP_MOVI16, 0, 0xFF, 0x7F, OP_MOV, 1, 0, 0, OP_HALT };
    flux_vm_execute(&vm, bc, sizeof(bc));
    EQ(vm.gp[0], 0); EQ(vm.gp[1], 0); PASS(); /* R0 ignores writes, reads as 0 */
}

void test_div_by_zero_fault() {
    TEST(div_by_zero_fault); FluxVM vm; flux_vm_init(&vm);
    uint8_t bc[] = { OP_MOVI,1,10, OP_MOVI,2,0, OP_DIV,3,1,2, OP_HALT };
    flux_vm_execute(&vm, bc, sizeof(bc));
    EQ(vm.faulted, true); EQ(vm.fault_code, 2); PASS();
}

void test_call_ret() {
    TEST(call_ret); FluxVM vm; flux_vm_init(&vm);
    /* CALL Format F: CALL rd, imm16 — push PC+4, jump to get_reg(rd)+imm16 */
    /* Byte layout: MOVI16(4) CALL(4) MOVI(3) MOVI(3) HALT(1) = 15 */
    uint8_t bc[] = {
        OP_MOVI16, 1, 0x0B, 0x00,     /* 0-3: R1 = 11 */
        OP_CALL, 1, 0x00, 0x00,        /* 4-7: push PC(8), jump to 11 */
        OP_MOVI, 2, 11,                /* 8-10: should be skipped */
        OP_MOVI, 2, 99,                /* 11-13: landed here */
        OP_HALT
    };
    flux_vm_execute(&vm, bc, sizeof(bc));
    EQ(vm.gp[2], 99); PASS();
}

void test_unknown_opcode_fault() {
    TEST(unknown_opcode_fault); FluxVM vm; flux_vm_init(&vm);
    uint8_t bc[] = { 0xFE, 0x00, 0x00, 0x00, OP_HALT };
    flux_vm_execute(&vm, bc, sizeof(bc));
    EQ(vm.faulted, true); EQ(vm.fault_code, 3); PASS();
}

void test_format_size() {
    TEST(format_size);
    EQ(flux_format_size(0x00), 1);  /* HALT = Format A */
    EQ(flux_format_size(0x0C), 2);  /* PUSH = Format B */
    EQ(flux_format_size(0x10), 2);  /* SYS = Format C */
    EQ(flux_format_size(0x18), 3);  /* MOVI = Format D */
    EQ(flux_format_size(0x20), 4);  /* ADD = Format E */
    EQ(flux_format_size(0x40), 4);  /* MOVI16 = Format F */
    EQ(flux_format_size(0x50), 5);  /* LOADI = Format G */
    EQ(flux_format_size(0x60), 4);  /* TELL = Format E */
    PASS();
}

int main() {
    printf("\nFLUX Unified VM Tests (FORMAT_A-G) — JetsonClaw1 Extended\n");
    printf("============================================================\n\n");

    /* Oracle1's 19 */
    test_init(); test_halt(); test_nop(); test_movi(); test_movi_negative();
    test_movi16(); test_movi16_negative(); test_add(); test_sub(); test_mul();
    test_div(); test_inc_dec(); test_push_pop(); test_jnz_loop();
    test_loop_instruction(); test_addi_subi(); test_cmp_eq();
    test_stripconf(); test_fibonacci();

    /* JetsonClaw1's extensions */
    test_load_store(); test_load_store_byte(); test_loadi_storei();
    test_conf_set_get(); test_conf_add_harmonic(); test_conf_fuse_bayesian();
    test_conf_clamp(); test_a2a_tell_stub(); test_a2a_fork();
    test_sensor_stubs(); test_atp_gen_use(); test_apoptosis();
    test_abs_sqrt(); test_rand(); test_swap(); test_r0_always_zero();
    test_div_by_zero_fault(); test_call_ret(); test_unknown_opcode_fault();
    test_format_size();

    printf("\n============================================================\n");
    printf("Results: %d passed, %d failed (total %d)\n", passed, failed, passed+failed);
    return failed > 0 ? 1 : 0;
}
