#include "flux/registers.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static int pass = 0, fail = 0;
#define TEST(name) printf("  TEST %-20s", name)
#define PASS() do { puts("PASS"); pass++; } while(0)
#define FAIL(msg) do { printf("FAIL: %s\n", msg); fail++; } while(0)

static void test_init(void) {
    TEST("init");
    FluxRegFile r;
    flux_regs_init(&r);
    for (int i = 0; i < 16; i++) {
        assert(r.gp[i] == 0);
        assert(r.fp[i] == 0.0f);
    }
    assert(r.pc == 0 && r.sp == 0);
    PASS();
}

static void test_gp_read_write(void) {
    TEST("gp_read_write");
    FluxRegFile r;
    flux_regs_init(&r);
    flux_gp_write(&r, 0, 42);
    assert(flux_gp_read(&r, 0) == 42);
    flux_gp_write(&r, 5, -100);
    assert(flux_gp_read(&r, 5) == -100);
    /* Out of bounds returns 0 */
    assert(flux_gp_read(&r, 20) == 0);
    PASS();
}

static void test_sp_sync(void) {
    TEST("sp_sync");
    FluxRegFile r;
    flux_regs_init(&r);
    flux_gp_write(&r, 11, 4096); /* R11 = SP */
    assert(r.sp == 4096);
    PASS();
}

static void test_fp_read_write(void) {
    TEST("fp_read_write");
    FluxRegFile r;
    flux_regs_init(&r);
    flux_fp_write(&r, 0, 3.14f);
    assert(flux_fp_read(&r, 0) == 3.14f);
    PASS();
}

int main(void) {
    printf("FluxRegFile tests\\n");
    test_init();
    test_gp_read_write();
    test_sp_sync();
    test_fp_read_write();
    printf("\\n%d/%d tests passed\\n", pass, pass + fail);
    return fail > 0 ? 1 : 0;
}
