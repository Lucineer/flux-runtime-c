#ifndef FLUX_VM_H
#define FLUX_VM_H
#include "flux/opcodes.h"
#include "flux/registers.h"
#include <stdint.h>
#include <stdbool.h>

#define FLUX_NUM_REGS    64
#define FLUX_STACK_SIZE  256
#define FLUX_MEMORY_SIZE 65536

typedef struct {
    int32_t  gp[FLUX_NUM_REGS];
    float    fp[FLUX_NUM_REGS];
    float    fconf[FLUX_NUM_REGS];  /* float confidence per register */
    int32_t  conf[FLUX_NUM_REGS];   /* integer confidence per register */
    uint8_t  vec[FLUX_NUM_REGS][16]; /* SIMD vectors */
    uint32_t pc, sp;
    int32_t  stack[FLUX_STACK_SIZE];
    uint8_t  memory[FLUX_MEMORY_SIZE];
    int32_t  cycles;
    bool     halted;
    bool     faulted;
    int32_t  fault_code;
    int32_t  zero_flag, neg_flag;
    int32_t  agent_id;
    int32_t  stripconf_remaining;
} FluxVM;

void     flux_vm_init(FluxVM* vm);
int32_t  flux_vm_execute(FluxVM* vm, const uint8_t* bytecode, int32_t len);
int32_t  flux_vm_step(FluxVM* vm, const uint8_t* bc, int32_t len);
int      flux_format_size(uint8_t opcode);
const char* flux_opcode_name(uint8_t op);

#endif
