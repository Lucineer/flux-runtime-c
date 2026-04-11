/* flux_vm_subroutine.c — Subroutine-threaded with forced inlining
 * Each opcode is a static inline function called via function pointer table.
 * The function pointer table is the dispatch mechanism.
 * 
 * Trade-offs:
 * + Cleaner separation of opcode logic (each is a named function)
 * + Easier to add profiling/hooks (wrap function pointers)
 * + More portable than computed goto (no GCC extension needed)
 * - Function pointer call is indirect branch (same as computed goto)
 * - Additional stack frame overhead if compiler refuses to inline
 * - Cannot use __attribute__((always_inline)) on function pointers
 * 
 * This variant teaches us: the overhead isn't the dispatch mechanism,
 * it's whether the compiler can ELIMINATE the dispatch entirely.
 */

#include "flux_vm.h"
#include <stdio.h>
#include <string.h>
#include <stdint.h>

#define LIKELY(x) __builtin_expect(!!(x), 1)

static inline int32_t get_reg(FluxVM *vm, int r) { return r==0 ? 0 : vm->gp[r&0x3F]; }
static inline void set_reg(FluxVM *vm, int r, int32_t v) { if(r!=0) vm->gp[r&0x3F]=v; }
static inline void update_flags(FluxVM *vm, int32_t v) { vm->zero_flag=(v==0); vm->neg_flag=(v<0); }

/* Decode context passed to each opcode handler */
typedef struct {
    FluxVM *vm;
    const uint8_t *bc;
    int pc;
    int len;
    int done;
} Ctx;

static inline void op_halt(Ctx *c) { c->vm->halted=1; c->done=1; }
static inline void op_nop(Ctx *c) { (void)c; }
static inline void op_inc(Ctx *c) { int r=c->bc[c->pc+1]; set_reg(c->vm,r,get_reg(c->vm,r)+1); update_flags(c->vm,get_reg(c->vm,r)); c->pc+=2; }
static inline void op_dec(Ctx *c) { int r=c->bc[c->pc+1]; set_reg(c->vm,r,get_reg(c->vm,r)-1); update_flags(c->vm,get_reg(c->vm,r)); c->pc+=2; }
static inline void op_push(Ctx *c) { int r=c->bc[c->pc+1]; c->vm->stack[--c->vm->sp]=get_reg(c->vm,r); c->pc+=2; }
static inline void op_pop(Ctx *c) { int r=c->bc[c->pc+1]; set_reg(c->vm,r,c->vm->stack[c->vm->sp++]); c->pc+=2; }
static inline void op_add(Ctx *c) { int rd=c->bc[c->pc+1],rs1=c->bc[c->pc+2],rs2=c->bc[c->pc+3]; set_reg(c->vm,rd,get_reg(c->vm,rs1)+get_reg(c->vm,rs2)); update_flags(c->vm,get_reg(c->vm,rd)); c->pc+=4; }
static inline void op_sub(Ctx *c) { int rd=c->bc[c->pc+1],rs1=c->bc[c->pc+2],rs2=c->bc[c->pc+3]; set_reg(c->vm,rd,get_reg(c->vm,rs1)-get_reg(c->vm,rs2)); update_flags(c->vm,get_reg(c->vm,rd)); c->pc+=4; }
static inline void op_mul(Ctx *c) { int rd=c->bc[c->pc+1],rs1=c->bc[c->pc+2],rs2=c->bc[c->pc+3]; set_reg(c->vm,rd,get_reg(c->vm,rs1)*get_reg(c->vm,rs2)); update_flags(c->vm,get_reg(c->vm,rd)); c->pc+=4; }
static inline void op_movi(Ctx *c) { int r=c->bc[c->pc+1]; int8_t imm=(int8_t)c->bc[c->pc+2]; set_reg(c->vm,r,(int32_t)imm); c->pc+=3; }
static inline void op_addi(Ctx *c) { int r=c->bc[c->pc+1]; int8_t imm=(int8_t)c->bc[c->pc+2]; set_reg(c->vm,r,get_reg(c->vm,r)+imm); update_flags(c->vm,get_reg(c->vm,r)); c->pc+=3; }
static inline void op_movi16(Ctx *c) { int r=c->bc[c->pc+1]; int16_t v; memcpy(&v,c->bc+c->pc+2,2); set_reg(c->vm,r,(int32_t)v); c->pc+=4; }
static inline void op_conf_add(Ctx *c) { int rd=c->bc[c->pc+1],rs1=c->bc[c->pc+2],rs2=c->bc[c->pc+3]; c->vm->conf[rd&0x3F]+=c->vm->conf[rs1&0x3F]; c->pc+=4; }
static inline void op_stub(Ctx *c) { c->pc += 2; }

typedef void (*OpHandler)(Ctx *);

int32_t flux_vm_execute(FluxVM *vm, const uint8_t *bc, int32_t len) {
    if (!vm || !bc || len <= 0 || vm->halted) return 0;
    
    static const OpHandler dt[256] = {
        [0x00]=(OpHandler)op_halt,[0x01]=(OpHandler)op_nop,
        [0x08]=(OpHandler)op_inc,[0x09]=(OpHandler)op_dec,
        [0x0C]=(OpHandler)op_push,[0x0D]=(OpHandler)op_pop,
        [0x18]=(OpHandler)op_movi,[0x19]=(OpHandler)op_addi,
        [0x20]=(OpHandler)op_add,[0x21]=(OpHandler)op_sub,[0x22]=(OpHandler)op_mul,
        [0x40]=(OpHandler)op_movi16,
        [0x70]=(OpHandler)op_conf_add,
    };
    
    Ctx ctx = {vm, bc, 0, len, 0};
    
    while (!ctx.done && ctx.pc < ctx.len) {
        uint8_t op = bc[ctx.pc];
        OpHandler h = dt[op];
        if (h) {
            h(&ctx);
        } else {
            ctx.pc += 4; /* unknown: skip Format E */
        }
    }
    
    vm->pc = ctx.pc;
    return ctx.pc;
}
