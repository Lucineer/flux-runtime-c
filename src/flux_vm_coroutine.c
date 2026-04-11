/* flux_vm_coroutine.c — Coroutine-threaded with yield/resume
 * Uses a state machine pattern where the VM can YIELD at any instruction
 * and RESUME from where it left off. This enables cooperative multitasking
 * — multiple VMs can interleave execution on a single thread.
 * 
 * Trade-offs:
 * + Enables fair scheduling without preemption
 * + VM state is always consistent at yield points
 * + Can implement time-slicing, instruction-count limits, debug breakpoints
 * + Natural fit for event-loop architectures
 * - State machine explosion (each opcode needs a RESUME state)
 * - Code size grows linearly with opcodes
 * - Slightly slower due to state variable checks
 * - This version uses a simplified counter-based yield
 * 
 * This variant teaches us about the COOPERATION dimension:
 * how execution models relate to agent coordination.
 */

#include "flux_vm.h"
#include <stdio.h>
#include <string.h>
#include <stdint.h>

#define LIKELY(x) __builtin_expect(!!(x), 1)

static inline int32_t get_reg(FluxVM *vm, int r) { return r==0 ? 0 : vm->gp[r&0x3F]; }
static inline void set_reg(FluxVM *vm, int r, int32_t v) { if(r!=0) vm->gp[r&0x3F]=v; }
static inline void update_flags(FluxVM *vm, int32_t v) { vm->zero_flag=(v==0); vm->neg_flag=(v<0); }

/* Execute up to `budget` instructions, then return.
 * Returns: number of instructions executed, or -1 if halted/faulted.
 * Call again with same vm to resume from where we left off.
 */
int32_t flux_vm_execute_budget(FluxVM *vm, const uint8_t *bc, int32_t len, int32_t budget) {
    if (!vm || !bc || len <= 0 || vm->halted) return -1;
    
    int pc = vm->pc;
    int32_t executed = 0;
    int32_t rd, rs1, rs2;
    uint8_t op;
    
    while (pc < len && executed < budget && !vm->halted && !vm->faulted) {
        op = bc[pc++];
        
        switch (op) {
        case 0x00: vm->halted=1; vm->pc=pc; return -1; /* HALT */
        case 0x01: break; /* NOP */
        case 0x08: rd=bc[pc++]; set_reg(vm,rd,get_reg(vm,rd)+1); update_flags(vm,get_reg(vm,rd)); break;
        case 0x09: rd=bc[pc++]; set_reg(vm,rd,get_reg(vm,rd)-1); update_flags(vm,get_reg(vm,rd)); break;
        case 0x0C: rd=bc[pc++]; vm->stack[--vm->sp]=get_reg(vm,rd); break;
        case 0x0D: rd=bc[pc++]; set_reg(vm,rd,vm->stack[vm->sp++]); break;
        case 0x18: rd=bc[pc++]; set_reg(vm,rd,(int32_t)(int8_t)bc[pc++]); break;
        case 0x19: rd=bc[pc++]; set_reg(vm,rd,get_reg(vm,rd)+(int8_t)bc[pc++]); update_flags(vm,get_reg(vm,rd)); break;
        case 0x20: rd=bc[pc++]; rs1=bc[pc++]; rs2=bc[pc++]; set_reg(vm,rd,get_reg(vm,rs1)+get_reg(vm,rs2)); update_flags(vm,get_reg(vm,rd)); break;
        case 0x21: rd=bc[pc++]; rs1=bc[pc++]; rs2=bc[pc++]; set_reg(vm,rd,get_reg(vm,rs1)-get_reg(vm,rs2)); update_flags(vm,get_reg(vm,rd)); break;
        case 0x22: rd=bc[pc++]; rs1=bc[pc++]; rs2=bc[pc++]; set_reg(vm,rd,get_reg(vm,rs1)*get_reg(vm,rs2)); update_flags(vm,get_reg(vm,rd)); break;
        case 0x23: rd=bc[pc++]; rs1=bc[pc++]; rs2=bc[pc++]; if(LIKELY(get_reg(vm,rs2)!=0)) set_reg(vm,rd,get_reg(vm,rs1)/get_reg(vm,rs2)); else { vm->faulted=1; vm->fault_code=2; } break;
        case 0x40: { rd=bc[pc++]; int16_t v; memcpy(&v,bc+pc,2); pc+=2; set_reg(vm,rd,(int32_t)v); break; }
        case 0x70: rd=bc[pc++]; rs1=bc[pc++]; rs2=bc[pc++]; vm->conf[rd&0x3F]+=vm->conf[rs1&0x3F]; break;
        default: vm->faulted=1; vm->fault_code=0xFF; vm->pc=pc-1; return -1;
        }
        
        executed++;
    }
    
    vm->pc = pc;
    return executed;
}
