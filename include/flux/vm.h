#ifndef FLUX_VM_H
#define FLUX_VM_H
#include "flux/opcodes.h"
#include "flux/registers.h"
#include "flux/memory.h"
#include <stdint.h>
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef enum {
    FLUX_OK=0, FLUX_ERR_HALT=1, FLUX_ERR_INVALID_OPCODE=2, FLUX_ERR_DIV_ZERO=3,
    FLUX_ERR_STACK_OVERFLOW=4, FLUX_ERR_STACK_UNDERFLOW=5, FLUX_ERR_CYCLE_BUDGET=11,
    FLUX_ERR_MEMORY=12
} FluxError;
#define FLUX_BOX_INT 0
#define FLUX_BOX_FLOAT 1
#define FLUX_BOX_MAX 64
typedef struct { int type_tag; int32_t int_val; float float_val; } FluxBox;
typedef struct FluxVM FluxVM;
typedef int (*FluxA2AHandler)(FluxVM*, uint8_t, const uint8_t*, uint16_t);
struct FluxVM {
    const uint8_t* bytecode; uint32_t bytecode_len;
    FluxRegFile regs; FluxMemManager mem;
    uint8_t flag_zero, flag_sign, flag_carry, flag_overflow;
    uint32_t cycle_count, max_cycles; uint8_t running, halted;
    FluxBox box_table[FLUX_BOX_MAX]; int box_count;
    uint8_t resources[256];
    uint32_t frame_stack[256]; uint32_t frame_count;
    FluxA2AHandler a2a_handler; void* user_data;
    FluxError last_error; uint32_t error_pc; uint8_t error_opcode;
};
int flux_vm_init(FluxVM* vm, const uint8_t* bytecode, uint32_t len, uint32_t mem_size);
void flux_vm_free(FluxVM* vm);
void flux_vm_reset(FluxVM* vm);
int64_t flux_vm_execute(FluxVM* vm);
int flux_vm_step(FluxVM* vm);
void flux_vm_set_a2a(FluxVM* vm, FluxA2AHandler h);
const char* flux_vm_error_string(FluxError err);
#ifdef __cplusplus
}
#endif
#endif
