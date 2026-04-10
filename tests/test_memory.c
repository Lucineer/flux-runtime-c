#include "flux.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>
static int pass=0, fail=0;
#define TEST(n) printf("  TEST %s... ", n)
#define PASS() do { printf("PASS\\n"); pass++; } while(0)
#define FAIL(m) do { printf("FAIL: %s\\n", m); fail++; } while(0)

static void test_init(void) {
    TEST("init");
    FluxMemManager m; flux_mem_init(&m);
    assert(m.count == 0); PASS();
}

static void test_create_region(void) {
    TEST("create_region");
    FluxMemManager m; flux_mem_init(&m);
    assert(flux_mem_create(&m, "heap", 1024, "agent") == 0);
    assert(m.count == 1);
    FluxMemRegion* r = flux_mem_get(&m, "heap");
    assert(r != NULL);
    assert(r->size == 1024);
    assert(strcmp(r->owner, "agent") == 0); PASS();
}

static void test_read_write(void) {
    TEST("read_write");
    FluxMemManager m; flux_mem_init(&m);
    flux_mem_create(&m, "heap", 4096, "agent");
    FluxMemRegion* r = flux_mem_get(&m, "heap");
    flux_mem_write_i32(r, 100, 0xDEADBEEF);
    assert(flux_mem_read_i32(r, 100) == (int32_t)0xDEADBEEF);
    flux_mem_write_u8(r, 200, 0x42);
    assert(flux_mem_read_u8(r, 200) == 0x42); PASS();
}

static void test_destroy(void) {
    TEST("destroy");
    FluxMemManager m; flux_mem_init(&m);
    flux_mem_create(&m, "temp", 512, "agent");
    assert(flux_mem_destroy(&m, "temp") == 0);
    assert(flux_mem_get(&m, "temp") == NULL); PASS();
}

static void test_max_regions(void) {
    TEST("max_regions");
    FluxMemManager m; flux_mem_init(&m);
    int ok = 1;
    char nm[16];
    for (int i = 0; i < 256; i++) {
        snprintf(nm, sizeof(nm), "r%d", i);
        if (flux_mem_create(&m, nm, 64, "agent") != 0) { ok = 0; break; }
    }
    /* 257th should fail */
    assert(flux_mem_create(&m, "overflow", 64, "agent") != 0);
    assert(ok); PASS();
}

int main(void) {
    printf("FLUX Memory Tests\\n\\n");
    test_init(); test_create_region(); test_read_write(); test_destroy(); test_max_regions();
    printf("\\n%d/%d tests passed\\n", pass, pass+fail);
    return fail;
}
