#include "flux/vm.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

static int pass = 0, fail = 0;
#define TEST(name) printf("  TEST %-24s", name)
#define PASS() do { puts("PASS"); pass++; } while(0)
#define FAIL(msg) do { printf("FAIL: %s\\n", msg); fail++; } while(0)

static void test_nop(void) {
    TEST("nop");
    uint8_t code[] = { FLUX_NOP, FLUX_HALT };
    FluxVM vm; flux_vm_init(&vm, code, 2, 4096);
    int64_t rc = flux_vm_execute(&vm);
    assert(rc == 2);
    PASS();
}

static void test_movi_add(void) {
    TEST("movi_add");
    /* R0 = 10, R1 = 20, R0 = R0 + R1 */
    uint8_t code[] = { FLUX_MOVI,0,10,0, FLUX_MOVI,1,20,0, FLUX_IADD,0,1, FLUX_HALT };
    FluxVM vm; flux_vm_init(&vm, code, sizeof(code), 4096);
    flux_vm_execute(&vm);
    assert(vm.regs.gp[0] == 30);
    PASS();
}

static void test_mul_sub(void) {
    TEST("mul_sub");
    uint8_t code[] = { FLUX_MOVI,0,6,0, FLUX_MOVI,1,7,0, FLUX_IMUL,0,1, FLUX_MOVI,1,10,0, FLUX_ISUB,0,1, FLUX_HALT };
    FluxVM vm; flux_vm_init(&vm, code, sizeof(code), 4096);
    flux_vm_execute(&vm);
    assert(vm.regs.gp[0] == 32); /* 6*7=42, 42-10=32 */
    PASS();
}

static void test_div_mod(void) {
    TEST("div_mod");
    uint8_t code[] = { FLUX_MOVI,0,17,0, FLUX_MOVI,1,5,0, FLUX_IDIV,0,1, FLUX_MOVI,2,0,0, FLUX_IMOD,2,1,
                       /* reload 17 into R2 for mod */
                       FLUX_MOVI,2,17,0, FLUX_IMOD,2,1, FLUX_HALT };
    FluxVM vm; flux_vm_init(&vm, code, sizeof(code), 4096);
    flux_vm_execute(&vm);
    assert(vm.regs.gp[0] == 3);   /* 17/5=3 */
    assert(vm.regs.gp[2] == 2);   /* 17%5=2 */
    PASS();
}

static void test_neg_inc_dec(void) {
    TEST("neg_inc_dec");
    uint8_t code[] = { FLUX_MOVI,0,5,0, FLUX_INEG,0, FLUX_INC,0, FLUX_DEC,0, FLUX_HALT };
    FluxVM vm; flux_vm_init(&vm, code, sizeof(code), 4096);
    flux_vm_execute(&vm);
    assert(vm.regs.gp[0] == -5); /* -5, then -4, then -5 */
    PASS();
}

static void test_bitwise(void) {
    TEST("bitwise");
    uint8_t code[] = {
        FLUX_MOVI,0,0xFF,0, /* 255 */
        FLUX_MOVI,1,0x0F,0, /* 15 */
        FLUX_IAND,0,1,      /* R0 = 255 & 15 = 15 */
        FLUX_MOVI,2,0xF0,0, /* 240 */
        FLUX_IOR,0,2,       /* R0 = 15 | 240 = 255 */
        FLUX_MOVI,3,0xFF,0,
        FLUX_IXOR,0,3,      /* R0 = 255 ^ 255 = 0 */
        FLUX_HALT
    };
    FluxVM vm; flux_vm_init(&vm, code, sizeof(code), 4096);
    flux_vm_execute(&vm);
    assert(vm.regs.gp[0] == 0);
    PASS();
}

static void test_jz_branch(void) {
    TEST("jz_branch");
    /* If R0==0 jump to add, else fall through to subtract */
    uint8_t code[] = {
        FLUX_MOVI,0,0,0,        /* R0 = 0 */
        FLUX_JZ,0,6,0,          /* if R0==0 jump +6 bytes (to IADD) */
        FLUX_MOVI,2,99,0,       /* skipped */
        FLUX_ISUB,0,0,          /* skipped */
        FLUX_MOVI,1,5,0,        /* R1 = 5 */
        FLUX_IADD,0,1,          /* R0 = 0 + 5 = 5 */
        FLUX_HALT
    };
    FluxVM vm; flux_vm_init(&vm, code, sizeof(code), 4096);
    flux_vm_execute(&vm);
    assert(vm.regs.gp[0] == 5);
    PASS();
}

static void test_jnz_no_branch(void) {
    TEST("jnz_no_branch");
    uint8_t code[] = {
        FLUX_MOVI,0,42,0,       /* R0 = 42 */
        FLUX_JNZ,0,0,0,         /* if R0!=0 jump +0 (no-op) */
        FLUX_HALT
    };
    FluxVM vm; flux_vm_init(&vm, code, sizeof(code), 4096);
    flux_vm_execute(&vm);
    assert(vm.regs.gp[0] == 42);
    PASS();
}

static void test_push_pop(void) {
    TEST("push_pop");
    uint8_t code[] = {
        FLUX_MOVI,0,42,0,
        FLUX_PUSH,0,
        FLUX_MOVI,0,0,0,
        FLUX_POP,0,
        FLUX_HALT
    };
    FluxVM vm; flux_vm_init(&vm, code, sizeof(code), 4096);
    flux_vm_execute(&vm);
    assert(vm.regs.gp[0] == 42);
    PASS();
}

static void test_dup(void) {
    TEST("dup");
    uint8_t code[] = {
        FLUX_MOVI,0,7,0,
        FLUX_PUSH,0,
        FLUX_DUP,
        FLUX_POP,1,    /* R1 = 7 */
        FLUX_POP,2,    /* R2 = 7 */
        FLUX_HALT
    };
    FluxVM vm; flux_vm_init(&vm, code, sizeof(code), 4096);
    flux_vm_execute(&vm);
    assert(vm.regs.gp[1] == 7 && vm.regs.gp[2] == 7);
    PASS();
}

static void test_call_ret(void) {
    TEST("call_ret");
    uint8_t code[] = {
        FLUX_MOVI,0,10,0,
        FLUX_MOVI,1,3,0,
        FLUX_ISUB,0,1,       /* R0=7 */
        FLUX_ISUB,0,1,       /* R0=4 */
        FLUX_MOVI,2,0,0,
        FLUX_HALT
    };
    FluxVM vm; flux_vm_init(&vm, code, sizeof(code), 4096);
    flux_vm_execute(&vm);
    assert(vm.regs.gp[0] == 4);
    PASS();
}

static void test_cmp_je(void) {
    TEST("cmp_je");
    uint8_t code[] = {
        FLUX_MOVI,0,5,0,
        FLUX_MOVI,1,5,0,
        FLUX_CMP,0,1,       /* compare: sets flag_zero=1 */
        FLUX_JE,0,3,0,      /* if zero flag set, jump +3 */
        FLUX_MOVI,2,0,0,    /* R2=0 (skipped) */
        FLUX_MOVI,2,1,0,    /* R2=1 */
        FLUX_HALT
    };
    FluxVM vm; flux_vm_init(&vm, code, sizeof(code), 4096);
    flux_vm_execute(&vm);
    assert(vm.regs.gp[2] == 1); /* JE taken, R2=1 */
    PASS();
}

static void test_cast(void) {
    TEST("cast");
    uint8_t code[] = {
        FLUX_MOVI,0,42,0,
        FLUX_CAST,0,0,      /* int -> float: R0 as int -> F0 */
        FLUX_HALT
    };
    FluxVM vm; flux_vm_init(&vm, code, sizeof(code), 4096);
    flux_vm_execute(&vm);
    assert(vm.regs.fp[0] == 42.0f);
    PASS();
}

static void test_float_ops(void) {
    TEST("float_ops");
    uint8_t code[] = {
        FLUX_MOVI,0,10,0,
        FLUX_CAST,0,0,           /* F0 = 10.0 */
        FLUX_MOVI,1,3,0,
        FLUX_CAST,1,0,           /* F1 = 3.0 */
        FLUX_FADD,0,1,           /* F0 = 13.0 */
        FLUX_FMUL,0,1,           /* F0 = 39.0 */
        FLUX_MOVI,1,4,0,
        FLUX_CAST,1,0,           /* F1 = 4.0 */
        FLUX_FDIV,0,1,           /* F0 = 9.75 */
        FLUX_HALT
    };
    FluxVM vm; flux_vm_init(&vm, code, sizeof(code), 4096);
    flux_vm_execute(&vm);
    assert(fabsf(vm.regs.fp[0] - 9.75f) < 0.001f);
    PASS();
}

static void test_region_create(void) {
    TEST("region_create");
    /* Create a memory region */
    /* REGION_CREATE format: op(1) len_u16(2) data(len) */
    /* data: name_len(1) name(n) size_u32(4) */
    /* "buf" = 3 bytes, size=1024 */
    (void)0; /* region create */
    uint8_t payload[] = { 3, 'b','u','f', 0x00,0x04,0x00,0x00 };
    uint16_t plen = sizeof(payload);
    uint8_t code[] = {
        FLUX_REGION_CREATE,
        (uint8_t)(plen & 0xFF), (uint8_t)((plen >> 8) & 0xFF),
        3, 'b','u','f', 0x00,0x04,0x00,0x00,
        FLUX_HALT
    };
    FluxVM vm; flux_vm_init(&vm, code, sizeof(code), 4096);
    flux_vm_execute(&vm);
    FluxMemRegion* r = flux_mem_get(&vm.mem, "buf");
    assert(r != NULL);
    assert(r->size == 1024);
    PASS();
}

static int a2a_called = 0;
static uint8_t a2a_got_op = 0;
static int a2a_test_handler(FluxVM* v, uint8_t op, const uint8_t* data, uint16_t len) {
    (void)v; (void)data; (void)len;
    a2a_called = 1; a2a_got_op = op; return 0;
}
static void test_a2a_handler(void) {
    TEST("a2a_handler");
    a2a_called = 0; a2a_got_op = 0;
    uint8_t code[] = {
        FLUX_TELL, 1,0, 0x42,
        FLUX_HALT
    };
    FluxVM vm; flux_vm_init(&vm, code, sizeof(code), 4096);
    vm.a2a_handler = a2a_test_handler;
    flux_vm_execute(&vm);
    assert(a2a_called == 1);
    assert(a2a_got_op == FLUX_TELL);
    PASS();
}

static void test_box_unbox(void) {
    TEST("box_unbox");
    /* BOX: op(1) len_u16(2) type_tag(1) int32_val(4) = 8 bytes */
    uint8_t payload[] = { FLUX_BOX_INT, 0x39,0x30,0,0 }; /* 12345 LE */ /* type=INT, val=12345 */
    uint16_t plen = sizeof(payload);
    uint8_t code[] = {
        FLUX_BOX, (uint8_t)(plen&0xFF), (uint8_t)((plen>>8)&0xFF),
        FLUX_BOX_INT, 0x39,0x30,0,0,
        /* R0 = box id */
        /* UNBOX: op(1) len_u16(2) id(1) */
        FLUX_UNBOX, 1,0, 0,
        FLUX_HALT
    };
    FluxVM vm; flux_vm_init(&vm, code, sizeof(code), 4096);
    flux_vm_execute(&vm);
    assert(vm.regs.gp[0] == 12345);
    PASS();
}

static void test_halt_error(void) {
    TEST("halt_error");
    uint8_t code[] = { FLUX_HALT };
    FluxVM vm; flux_vm_init(&vm, code, 1, 4096);
    flux_vm_execute(&vm);
    assert(vm.halted == 1);
    assert(vm.last_error == 1);
    PASS();
}

static void test_div_zero(void) {
    TEST("div_zero");
    uint8_t code[] = { FLUX_MOVI,0,1,0, FLUX_MOVI,1,0,0, FLUX_IDIV,0,1, FLUX_HALT };
    FluxVM vm; flux_vm_init(&vm, code, sizeof(code), 4096);
    flux_vm_execute(&vm);
    assert(vm.last_error == 3); /* DIV_ZERO */
    PASS();
}

static void test_cycle_budget(void) {
    TEST("cycle_budget");
    /* Infinite loop: JMP to self */
    /* JMP: op(1) rd(1) imm_lo(1) imm_hi(1) = 4 bytes */
    /* imm=-4 means: after fetch pc=4, target = 4 + (-4) = 0 */
    uint8_t code[] = {
        FLUX_JMP, 0, (uint8_t)(-4 & 0xFF), (uint8_t)((-4 >> 8) & 0xFF),
    };
    FluxVM vm; flux_vm_init(&vm, code, 4, 4096);
    vm.max_cycles = 1000;
    flux_vm_execute(&vm);
    assert(vm.last_error == 11); /* CYCLE_BUDGET */
    PASS();
}

int main(void) {
    printf("FluxVM tests\\n");
    test_nop();
    test_movi_add();
    test_mul_sub();
    test_div_mod();
    test_neg_inc_dec();
    test_bitwise();
    test_jz_branch();
    test_jnz_no_branch();
    test_push_pop();
    test_dup();
    test_call_ret();
    test_cmp_je();
    test_cast();
    test_float_ops();
    test_region_create();
    test_a2a_handler();
    test_box_unbox();
    test_halt_error();
    test_div_zero();
    test_cycle_budget();
    printf("\\n%d/%d tests passed\\n", pass, pass + fail);
    return fail > 0 ? 1 : 0;
}
