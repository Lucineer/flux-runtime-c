/* flux_vm_token.c — Token-threaded interpreter
 * Bytecode is pre-decoded into a flat instruction struct array.
 * Each instruction: { uint8_t opcode, int32_t rd, int32_t rs1, int32_t rs2 }
 * No per-instruction decoding during execution.
 * 
 * Trade-offs:
 * + Zero decode overhead per instruction (everything pre-resolved)
 * + Better cache locality for operand fields (struct layout)
 * + Enables pre-optimization passes on the token stream
 * - One-time decode cost before execution
 * - 16 bytes per instruction vs 1-4 bytes raw bytecode
 * - Needs token array allocation (heap or stack)
 * 
 * When to use: when executing the same bytecode many times (loops, hot paths)
 * When NOT to use: one-shot execution, memory-constrained
 */

#include "flux_vm.h"
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>

#define LIKELY(x) __builtin_expect(!!(x), 1)
#define UNLIKELY(x) __builtin_expect(!!(x), 0)

typedef struct {
    uint8_t op;
    int32_t a, b, c;
} Token;

static inline int32_t get_reg(FluxVM *vm, int r) { return r==0 ? 0 : vm->gp[r&0x3F]; }
static inline void set_reg(FluxVM *vm, int r, int32_t v) { if(r!=0) vm->gp[r&0x3F]=v; }
static inline void update_flags(FluxVM *vm, int32_t v) { vm->zero_flag=(v==0); vm->neg_flag=(v<0); }

/* Pre-decode bytecode into token stream. Returns number of tokens. */
int flux_decode_tokens(const uint8_t *bc, int32_t len, Token **out) {
    if (!bc || len <= 0) return 0;
    
    /* Count instructions first */
    int count = 0, pc = 0;
    while (pc < len) {
        uint8_t op = bc[pc];
        int size = 1;
        if (op <= 0x07) size = 1;      /* Format A */
        else if (op <= 0x0F) size = 2;  /* Format B */
        else if (op <= 0x17) size = 2;  /* Format C */
        else if (op <= 0x1F) size = 3;  /* Format D */
        else if (op <= 0x5F) size = 4;  /* Format E/F */
        else size = 4;                  /* Format E extended */
        pc += size;
        count++;
    }
    
    Token *tokens = (Token *)malloc(count * sizeof(Token));
    if (!tokens) return 0;
    
    /* Decode */
    pc = 0;
    int ti = 0;
    while (pc < len) {
        uint8_t op = bc[pc];
        tokens[ti].op = op;
        tokens[ti].a = tokens[ti].b = tokens[ti].c = 0;
        
        if (op <= 0x07) { pc += 1; }
        else if (op <= 0x0F) { tokens[ti].a = bc[pc+1]; pc += 2; }
        else if (op <= 0x17) { tokens[ti].a = bc[pc+1]; tokens[ti].b = (int8_t)bc[pc+2]; pc += 2; }
        else if (op <= 0x1F) { tokens[ti].a = bc[pc+1]; tokens[ti].b = (int8_t)bc[pc+2]; pc += 3; }
        else { tokens[ti].a = bc[pc+1]; tokens[ti].b = bc[pc+2]; tokens[ti].c = bc[pc+3]; pc += 4; }
        ti++;
    }
    
    *out = tokens;
    return count;
}

int32_t flux_vm_execute_tokens(FluxVM *vm, const Token *tokens, int count) {
    if (!vm || !tokens || count <= 0 || vm->halted) return 0;
    
    int32_t rd, rs1, rs2;
    
    static const void *dt[256] = {
        [0x00]=&&H,[0x01]=&&N,[0x08]=&&INC,[0x09]=&&DEC,[0x0C]=&&PUSH,[0x0D]=&&POP,
        [0x0E]=&&CLD,[0x0F]=&&CST,
        [0x18]=&&MOVI,[0x19]=&&ADDI,[0x1A]=&&SUBI,[0x1B]=&&ANDI,
        [0x1C]=&&ORI,[0x1D]=&&XORI,[0x1E]=&&SHLI,[0x1F]=&&SHRI,
        [0x20]=&&ADD,[0x21]=&&SUB,[0x22]=&&MUL,[0x23]=&&DIV,[0x24]=&&MOD,
        [0x25]=&&AND,[0x26]=&&OR,[0x27]=&&XOR,[0x28]=&&SHL,[0x29]=&&SHR,
        [0x2A]=&&EQ,[0x2B]=&&NE,[0x2C]=&&LT,[0x2D]=&&GT,[0x2E]=&&LE,[0x2F]=&&GE,
        [0x34]=&&LD,[0x35]=&&ST,[0x36]=&&MR,[0x37]=&&SWP,
        [0x40]=&&M16,[0x42]=&&CMPZ,[0x43]=&&CMPR,
        [0x50]=&&ABS,[0x51]=&&MIN,[0x52]=&&MAX,
        [0x70]=&&CADD,[0x71]=&&CSUB,[0x72]=&&CMUL,[0x73]=&&CFUSE,
    };
    
    const Token *tp = tokens;
    const Token *end = tokens + count;
    
    #define TD() goto *dt[tp->op]
    
    TD();
    
    H: vm->halted=1; return (int32_t)(tp - tokens);
    N: tp++; TD();
    INC: set_reg(vm,tp->a,get_reg(vm,tp->a)+1); update_flags(vm,get_reg(vm,tp->a)); tp++; TD();
    DEC: set_reg(vm,tp->a,get_reg(vm,tp->a)-1); update_flags(vm,get_reg(vm,tp->a)); tp++; TD();
    PUSH: vm->stack[--vm->sp]=get_reg(vm,tp->a); tp++; TD();
    POP: set_reg(vm,tp->a,vm->stack[vm->sp++]); tp++; TD();
    CLD: set_reg(vm,tp->a,vm->conf[tp->a&0x3F]); tp++; TD();
    CST: vm->conf[tp->a&0x3F]=get_reg(vm,tp->a); tp++; TD();
    MOVI: set_reg(vm,tp->a,(int32_t)(int8_t)tp->b); tp++; TD();
    ADDI: set_reg(vm,tp->a,get_reg(vm,tp->a)+(int8_t)tp->b); update_flags(vm,get_reg(vm,tp->a)); tp++; TD();
    SUBI: set_reg(vm,tp->a,get_reg(vm,tp->a)-(int8_t)tp->b); update_flags(vm,get_reg(vm,tp->a)); tp++; TD();
    ANDI: set_reg(vm,tp->a,get_reg(vm,tp->a)&tp->b); tp++; TD();
    ORI: set_reg(vm,tp->a,get_reg(vm,tp->a)|tp->b); tp++; TD();
    XORI: set_reg(vm,tp->a,get_reg(vm,tp->a)^tp->b); tp++; TD();
    SHLI: set_reg(vm,tp->a,get_reg(vm,tp->a)<<(int8_t)tp->b); tp++; TD();
    SHRI: set_reg(vm,tp->a,get_reg(vm,tp->a)>>(int8_t)tp->b); tp++; TD();
    ADD: set_reg(vm,tp->a,get_reg(vm,tp->b)+get_reg(vm,tp->c)); update_flags(vm,get_reg(vm,tp->a)); tp++; TD();
    SUB: set_reg(vm,tp->a,get_reg(vm,tp->b)-get_reg(vm,tp->c)); update_flags(vm,get_reg(vm,tp->a)); tp++; TD();
    MUL: set_reg(vm,tp->a,get_reg(vm,tp->b)*get_reg(vm,tp->c)); update_flags(vm,get_reg(vm,tp->a)); tp++; TD();
    DIV: if(LIKELY(get_reg(vm,tp->c)!=0)) set_reg(vm,tp->a,get_reg(vm,tp->b)/get_reg(vm,tp->c)); else { vm->faulted=1; vm->fault_code=2; } tp++; TD();
    MOD: if(LIKELY(get_reg(vm,tp->c)!=0)) set_reg(vm,tp->a,get_reg(vm,tp->b)%get_reg(vm,tp->c)); else { vm->faulted=1; vm->fault_code=2; } tp++; TD();
    AND: set_reg(vm,tp->a,get_reg(vm,tp->b)&get_reg(vm,tp->c)); tp++; TD();
    OR: set_reg(vm,tp->a,get_reg(vm,tp->b)|get_reg(vm,tp->c)); tp++; TD();
    XOR: set_reg(vm,tp->a,get_reg(vm,tp->b)^get_reg(vm,tp->c)); tp++; TD();
    SHL: set_reg(vm,tp->a,get_reg(vm,tp->b)<<get_reg(vm,tp->c)); tp++; TD();
    SHR: set_reg(vm,tp->a,get_reg(vm,tp->b)>>get_reg(vm,tp->c)); tp++; TD();
    EQ: set_reg(vm,tp->a,get_reg(vm,tp->b)==get_reg(vm,tp->c)); update_flags(vm,get_reg(vm,tp->a)); tp++; TD();
    NE: set_reg(vm,tp->a,get_reg(vm,tp->b)!=get_reg(vm,tp->c)); update_flags(vm,get_reg(vm,tp->a)); tp++; TD();
    LT: set_reg(vm,tp->a,get_reg(vm,tp->b)<get_reg(vm,tp->c)); update_flags(vm,get_reg(vm,tp->a)); tp++; TD();
    GT: set_reg(vm,tp->a,get_reg(vm,tp->b)>get_reg(vm,tp->c)); update_flags(vm,get_reg(vm,tp->a)); tp++; TD();
    LE: set_reg(vm,tp->a,get_reg(vm,tp->b)<=get_reg(vm,tp->c)); update_flags(vm,get_reg(vm,tp->a)); tp++; TD();
    GE: set_reg(vm,tp->a,get_reg(vm,tp->b)>=get_reg(vm,tp->c)); update_flags(vm,tp->a)); tp++; TD();
    LD: set_reg(vm,tp->a,(int32_t)vm->memory[(get_reg(vm,tp->b)+get_reg(vm,tp->c))&0xFFFF]); tp++; TD();
    ST: vm->memory[(get_reg(vm,tp->b)+get_reg(vm,tp->c))&0xFFFF]=(uint8_t)get_reg(vm,tp->a); tp++; TD();
    MR: set_reg(vm,tp->a,get_reg(vm,tp->b)); tp++; TD();
    SWP: { int32_t t=get_reg(vm,tp->a); set_reg(vm,tp->a,get_reg(vm,tp->b)); set_reg(vm,tp->b,t); } tp++; TD();
    M16: set_reg(vm,tp->a,(int32_t)(int16_t)(tp->b|(tp->c<<8))); tp++; TD();
    CMPZ: update_flags(vm,get_reg(vm,tp->a)-get_reg(vm,tp->b)); tp++; TD();
    CMPR: { int32_t c=get_reg(vm,tp->b)-get_reg(vm,tp->c); update_flags(vm,c); set_reg(vm,tp->a,c<0?-1:c>0?1:0); } tp++; TD();
    ABS: set_reg(vm,tp->a,get_reg(vm,tp->b)<0?-get_reg(vm,tp->b):get_reg(vm,tp->b)); tp++; TD();
    MIN: set_reg(vm,tp->a,get_reg(vm,tp->b)<get_reg(vm,tp->c)?get_reg(vm,tp->b):get_reg(vm,tp->c)); tp++; TD();
    MAX: set_reg(vm,tp->a,get_reg(vm,tp->b)>get_reg(vm,tp->c)?get_reg(vm,tp->b):get_reg(vm,tp->c)); tp++; TD();
    CADD: vm->conf[tp->a&0x3F]+=vm->conf[tp->b&0x3F]; tp++; TD();
    CSUB: vm->conf[tp->a&0x3F]-=vm->conf[tp->b&0x3F]; tp++; TD();
    CMUL: vm->conf[tp->a&0x3F]*=vm->conf[tp->b&0x3F]; tp++; TD();
    CFUSE: { int32_t cs=vm->conf[tp->b&0x3F]+vm->conf[tp->c&0x3F]; vm->conf[tp->a&0x3F]=cs?((get_reg(vm,tp->b)*vm->conf[tp->b&0x3F]+get_reg(vm,tp->c)*vm->conf[tp->c&0x3F])/cs):0; } tp++; TD();
    
    vm->faulted=1; vm->fault_code=0xFF; return (int32_t)(tp - tokens);
    #undef TD
}
