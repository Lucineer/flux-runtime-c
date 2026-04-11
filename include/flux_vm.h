#ifndef FLUX_VM_H
#define FLUX_VM_H

#include "formats.h"
#include <stdint.h>
#include <stdbool.h>

typedef struct {
    int32_t gp[FLUX_NUM_REGS];       /* General purpose registers */
    int32_t conf[FLUX_NUM_REGS];     /* Confidence registers (parallel to GP) */
    float   fp[FLUX_NUM_REGS];       /* Float registers */
    int32_t zero_flag;               /* Zero flag */
    int32_t neg_flag;                /* Negative flag */
    int32_t stack[FLUX_STACK_SIZE];  /* Data stack */
    int32_t sp;                      /* Stack pointer (grows down) */
    int32_t pc;                      /* Program counter */
    uint8_t memory[FLUX_MEMORY_SIZE];/* 64KB flat memory */
    bool halted;
    bool faulted;
    int32_t fault_code;
    int32_t cycles;
    int32_t agent_id;                /* Agent ID for A2A ops */
    int32_t stripconf_remaining;     /* StripConf counter */
} FluxVM;

void flux_vm_init(FluxVM* vm);
int32_t flux_vm_execute(FluxVM* vm, const uint8_t* bytecode, int32_t len);
int32_t flux_vm_step(FluxVM* vm, const uint8_t* bytecode, int32_t len);
int flux_format_size(uint8_t opcode);

#endif /* FLUX_VM_H */
