#include "flux/vm.h"
#include <stdio.h>
#include <string.h>
int main(void) {
    /* Simple demo: R0 = 3, R1 = 4, R0 = R0 * R1 */
    uint8_t code[] = {
        0x0B, 0, 3, 0,   /* MOVI R0, 3 */
        0x0B, 1, 4, 0,   /* MOVI R1, 4 */
        0x0A, 0, 1,      /* IMUL R0, R1 */
        0x80,             /* HALT */
    };
    /* Fix: use proper opcodes */
    code[0] = FLUX_MOVI; code[4] = FLUX_MOVI; code[8] = FLUX_IMUL; code[11] = FLUX_HALT;
    FluxVM vm;
    flux_vm_init(&vm, code, sizeof(code), 4096);
    printf("FLUX Runtime C — Micro-VM Demo\\n");
    printf("Bytecode: %zu bytes, 85 opcodes, A2A protocol, SIMD\\n", sizeof(code));
    int64_t cycles = flux_vm_execute(&vm);
    printf("Result: R0 = %d (expected 12)\\n", vm.regs.gp[0]);
    printf("Cycles: %lld\\n", (long long)cycles);
    flux_vm_free(&vm);
    return vm.regs.gp[0] == 12 ? 0 : 1;
}
