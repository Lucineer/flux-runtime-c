/*
 * flux_vm_cg.c — Computed goto dispatch for FLUX VM (Iteration 3)
 */
#include "flux_vm.h"
#include <stdio.h>
#include <string.h>
#include <stdint.h>

#define LIKELY(x)   __builtin_expect(!!(x), 1)
#define UNLIKELY(x) __builtin_expect(!!(x), 0)

static inline int32_t get_reg(FluxVM *vm, int r) { return r==0 ? 0 : vm->gp[r&0x3F]; }
static inline void set_reg(FluxVM *vm, int r, int32_t v) { if(r!=0) vm->gp[r&0x3F]=v; }
static inline void update_flags(FluxVM *vm, int32_t v) { vm->zero_flag=(v==0); vm->neg_flag=(v<0); }

int32_t flux_vm_execute(FluxVM *vm, const uint8_t *bc, int32_t len) {
    if (!vm || !bc || len <= 0 || vm->halted) return 0;
    
    int pc = vm->pc;
    int32_t rd, rs1, rs2;
    int16_t imm16;
    int8_t imm8;
    uint8_t op;
    
    static const void *dt[256] = {
        [0x00]=&&L_HALT,[0x01]=&&L_NOP,[0x02]=&&L_RET,[0x04]=&&L_BRK,
        [0x06]=&&L_RST,
        [0x08]=&&L_INC,[0x09]=&&L_DEC,[0x0A]=&&L_NOT,[0x0B]=&&L_NEG,
        [0x0C]=&&L_PUSH,[0x0D]=&&L_POP,[0x0E]=&&L_CLD,[0x0F]=&&L_CST,
        [0x12]=&&L_DBG,[0x13]=&&L_CLF,[0x15]=&&L_YLD,[0x17]=&&L_STRIP,
        [0x18]=&&L_MOVI,[0x19]=&&L_ADDI,[0x1A]=&&L_SUBI,[0x1B]=&&L_ANDI,
        [0x1C]=&&L_ORI,[0x1D]=&&L_XORI,[0x1E]=&&L_SHLI,[0x1F]=&&L_SHRI,
        [0x20]=&&L_ADD,[0x21]=&&L_SUB,[0x22]=&&L_MUL,[0x23]=&&L_DIV,
        [0x24]=&&L_MOD,[0x25]=&&L_AND,[0x26]=&&L_OR,[0x27]=&&L_XOR,
        [0x28]=&&L_SHL,[0x29]=&&L_SHR,[0x2A]=&&L_EQ,[0x2B]=&&L_NE,
        [0x2C]=&&L_LT,[0x2D]=&&L_GT,[0x2E]=&&L_LE,[0x2F]=&&L_GE,
        [0x34]=&&L_LD,[0x35]=&&L_ST,[0x36]=&&L_MR,[0x37]=&&L_SWP,
        [0x38]=&&L_CALL,[0x3A]=&&L_LOOP,[0x3B]=&&L_JEQ,[0x3C]=&&L_JNE,
        [0x3D]=&&L_JLT,[0x3E]=&&L_JGT,[0x3F]=&&L_JLE,
        [0x40]=&&L_M16,[0x42]=&&L_CMPZ,[0x43]=&&L_CMPR,[0x44]=&&L_FENCE,
        [0x50]=&&L_ABS,[0x51]=&&L_MIN,[0x52]=&&L_MAX,
        [0x70]=&&L_CADD,[0x71]=&&L_CSUB,[0x72]=&&L_CMUL,[0x73]=&&L_CFUSE,
    };
    
    #define D() goto *dt[op = bc[pc++]]
    #define D_BAD() do { vm->faulted=1; vm->fault_code=0xFF; vm->pc=pc-1; return pc-1; } while(0)
    
    if (UNLIKELY(pc >= len)) { vm->halted=1; vm->pc=pc; return pc; }
    D();
    
    L_HALT: vm->halted=1; vm->pc=pc; return pc;
    L_NOP:  D();
    L_RET:  if(vm->sp<FLUX_STACK_SIZE) pc=vm->stack[vm->sp++]; else { vm->faulted=1; vm->fault_code=3; } D();
    L_BRK:  vm->halted=1; vm->pc=pc; return pc;
    L_RST:  memset(vm->gp,0,sizeof(vm->gp)); vm->sp=FLUX_STACK_SIZE; vm->zero_flag=vm->neg_flag=0; D();
    L_INC:  rd=bc[pc++]; set_reg(vm,rd,get_reg(vm,rd)+1); update_flags(vm,get_reg(vm,rd)); D();
    L_DEC:  rd=bc[pc++]; set_reg(vm,rd,get_reg(vm,rd)-1); update_flags(vm,get_reg(vm,rd)); D();
    L_NOT:  rd=bc[pc++]; set_reg(vm,rd,~get_reg(vm,rd)); D();
    L_NEG:  rd=bc[pc++]; set_reg(vm,rd,-get_reg(vm,rd)); D();
    L_PUSH: rd=bc[pc++]; vm->stack[--vm->sp]=get_reg(vm,rd); D();
    L_POP:  rd=bc[pc++]; set_reg(vm,rd,vm->stack[vm->sp++]); D();
    L_CLD:  rd=bc[pc++]; set_reg(vm,rd,vm->conf[rd&0x3F]); D();
    L_CST:  rd=bc[pc++]; vm->conf[rd&0x3F]=get_reg(vm,rd); D();
    L_DBG:  rd=bc[pc++]; pc++; fprintf(stderr,"r%d=%d\n",rd,get_reg(vm,rd)); D();
    L_CLF:  pc++; vm->zero_flag=0; vm->neg_flag=0; D();
    L_YLD:  pc++; D();
    L_STRIP: vm->stripconf_remaining=bc[pc++]; D();
    L_MOVI: rd=bc[pc++]; imm8=(int8_t)bc[pc++]; set_reg(vm,rd,(int32_t)imm8); D();
    L_ADDI: rd=bc[pc++]; imm8=(int8_t)bc[pc++]; set_reg(vm,rd,get_reg(vm,rd)+imm8); update_flags(vm,get_reg(vm,rd)); D();
    L_SUBI: rd=bc[pc++]; imm8=(int8_t)bc[pc++]; set_reg(vm,rd,get_reg(vm,rd)-imm8); update_flags(vm,get_reg(vm,rd)); D();
    L_ANDI: rd=bc[pc++]; set_reg(vm,rd,get_reg(vm,rd)&bc[pc++]); D();
    L_ORI:  rd=bc[pc++]; set_reg(vm,rd,get_reg(vm,rd)|bc[pc++]); D();
    L_XORI: rd=bc[pc++]; set_reg(vm,rd,get_reg(vm,rd)^bc[pc++]); D();
    L_SHLI: rd=bc[pc++]; imm8=(int8_t)bc[pc++]; set_reg(vm,rd,get_reg(vm,rd)<<imm8); D();
    L_SHRI: rd=bc[pc++]; imm8=(int8_t)bc[pc++]; set_reg(vm,rd,get_reg(vm,rd)>>imm8); D();
    L_ADD:  rd=bc[pc++]; rs1=bc[pc++]; rs2=bc[pc++]; set_reg(vm,rd,get_reg(vm,rs1)+get_reg(vm,rs2)); update_flags(vm,get_reg(vm,rd)); D();
    L_SUB:  rd=bc[pc++]; rs1=bc[pc++]; rs2=bc[pc++]; set_reg(vm,rd,get_reg(vm,rs1)-get_reg(vm,rs2)); update_flags(vm,get_reg(vm,rd)); D();
    L_MUL:  rd=bc[pc++]; rs1=bc[pc++]; rs2=bc[pc++]; set_reg(vm,rd,get_reg(vm,rs1)*get_reg(vm,rs2)); update_flags(vm,get_reg(vm,rd)); D();
    L_DIV:  rd=bc[pc++]; rs1=bc[pc++]; rs2=bc[pc++]; if(LIKELY(get_reg(vm,rs2)!=0)) set_reg(vm,rd,get_reg(vm,rs1)/get_reg(vm,rs2)); else { vm->faulted=1; vm->fault_code=2; } D();
    L_MOD:  rd=bc[pc++]; rs1=bc[pc++]; rs2=bc[pc++]; if(LIKELY(get_reg(vm,rs2)!=0)) set_reg(vm,rd,get_reg(vm,rs1)%get_reg(vm,rs2)); else { vm->faulted=1; vm->fault_code=2; } D();
    L_AND:  rd=bc[pc++]; rs1=bc[pc++]; rs2=bc[pc++]; set_reg(vm,rd,get_reg(vm,rs1)&get_reg(vm,rs2)); D();
    L_OR:   rd=bc[pc++]; rs1=bc[pc++]; rs2=bc[pc++]; set_reg(vm,rd,get_reg(vm,rs1)|get_reg(vm,rs2)); D();
    L_XOR:  rd=bc[pc++]; rs1=bc[pc++]; rs2=bc[pc++]; set_reg(vm,rd,get_reg(vm,rs1)^get_reg(vm,rs2)); D();
    L_SHL:  rd=bc[pc++]; rs1=bc[pc++]; rs2=bc[pc++]; set_reg(vm,rd,get_reg(vm,rs1)<<get_reg(vm,rs2)); D();
    L_SHR:  rd=bc[pc++]; rs1=bc[pc++]; rs2=bc[pc++]; set_reg(vm,rd,get_reg(vm,rs1)>>get_reg(vm,rs2)); D();
    L_EQ:   rd=bc[pc++]; rs1=bc[pc++]; rs2=bc[pc++]; set_reg(vm,rd,get_reg(vm,rs1)==get_reg(vm,rs2)); update_flags(vm,get_reg(vm,rd)); D();
    L_NE:   rd=bc[pc++]; rs1=bc[pc++]; rs2=bc[pc++]; set_reg(vm,rd,get_reg(vm,rs1)!=get_reg(vm,rs2)); update_flags(vm,get_reg(vm,rd)); D();
    L_LT:   rd=bc[pc++]; rs1=bc[pc++]; rs2=bc[pc++]; set_reg(vm,rd,get_reg(vm,rs1)<get_reg(vm,rs2)); update_flags(vm,get_reg(vm,rd)); D();
    L_GT:   rd=bc[pc++]; rs1=bc[pc++]; rs2=bc[pc++]; set_reg(vm,rd,get_reg(vm,rs1)>get_reg(vm,rs2)); update_flags(vm,get_reg(vm,rd)); D();
    L_LE:   rd=bc[pc++]; rs1=bc[pc++]; rs2=bc[pc++]; set_reg(vm,rd,get_reg(vm,rs1)<=get_reg(vm,rs2)); update_flags(vm,get_reg(vm,rd)); D();
    L_GE:   rd=bc[pc++]; rs1=bc[pc++]; rs2=bc[pc++]; set_reg(vm,rd,get_reg(vm,rs1)>=get_reg(vm,rs2)); update_flags(vm,get_reg(vm,rd)); D();
    L_LD:   rd=bc[pc++]; rs1=bc[pc++]; rs2=bc[pc++]; set_reg(vm,rd,(int32_t)bc[(get_reg(vm,rs1)+get_reg(vm,rs2))&0xFFFF]); D();
    L_ST:   rd=bc[pc++]; rs1=bc[pc++]; rs2=bc[pc++]; vm->memory[(get_reg(vm,rs1)+get_reg(vm,rs2))&0xFFFF]=(uint8_t)get_reg(vm,rd); D();
    L_MR:   rd=bc[pc++]; rs1=bc[pc++]; pc++; set_reg(vm,rd,get_reg(vm,rs1)); D();
    L_SWP:  rd=bc[pc++]; rs1=bc[pc++]; pc++; { int32_t t=get_reg(vm,rd); set_reg(vm,rd,get_reg(vm,rs1)); set_reg(vm,rs1,t); } D();
    L_CALL: vm->stack[--vm->sp]=(int32_t)(pc+3); rd=bc[pc++]; rs1=bc[pc++]; rs2=bc[pc++]; pc+=get_reg(vm,rd); D();
    L_LOOP: rd=bc[pc++]; memcpy(&imm16,bc+pc,2); pc+=2; set_reg(vm,rd,get_reg(vm,rd)-1); if(LIKELY(get_reg(vm,rd)>0)) pc+=(int32_t)imm16; D();
    L_JEQ:  rd=bc[pc++]; memcpy(&imm16,bc+pc,2); pc+=2; if(get_reg(vm,rd)==0) pc+=(int32_t)imm16; D();
    L_JNE:  rd=bc[pc++]; memcpy(&imm16,bc+pc,2); pc+=2; if(get_reg(vm,rd)!=0) pc+=(int32_t)imm16; D();
    L_JLT:  rd=bc[pc++]; memcpy(&imm16,bc+pc,2); pc+=2; if(get_reg(vm,rd)<0) pc+=(int32_t)imm16; D();
    L_JGT:  rd=bc[pc++]; memcpy(&imm16,bc+pc,2); pc+=2; if(get_reg(vm,rd)>0) pc+=(int32_t)imm16; D();
    L_JLE:  rd=bc[pc++]; memcpy(&imm16,bc+pc,2); pc+=2; if(get_reg(vm,rd)<=0) pc+=(int32_t)imm16; D();
    L_M16:  rd=bc[pc++]; memcpy(&imm16,bc+pc,2); pc+=2; set_reg(vm,rd,(int32_t)imm16); D();
    L_CMPZ: rd=bc[pc++]; rs1=bc[pc++]; rs2=bc[pc++]; update_flags(vm,get_reg(vm,rd)-get_reg(vm,rs1)); D();
    L_CMPR: rd=bc[pc++]; rs1=bc[pc++]; rs2=bc[pc++]; { int32_t c=get_reg(vm,rs1)-get_reg(vm,rs2); update_flags(vm,c); set_reg(vm,rd,c<0?-1:c>0?1:0); } D();
    L_FENCE: pc+=3; D();
    L_ABS:  rd=bc[pc++]; rs1=bc[pc++]; pc++; set_reg(vm,rd,get_reg(vm,rs1)<0?-get_reg(vm,rs1):get_reg(vm,rs1)); D();
    L_MIN:  rd=bc[pc++]; rs1=bc[pc++]; rs2=bc[pc++]; set_reg(vm,rd,get_reg(vm,rs1)<get_reg(vm,rs2)?get_reg(vm,rs1):get_reg(vm,rs2)); D();
    L_MAX:  rd=bc[pc++]; rs1=bc[pc++]; rs2=bc[pc++]; set_reg(vm,rd,get_reg(vm,rs1)>get_reg(vm,rs2)?get_reg(vm,rs1):get_reg(vm,rs2)); D();
    L_CADD: rd=bc[pc++]; rs1=bc[pc++]; rs2=bc[pc++]; vm->conf[rd&0x3F]+=vm->conf[rs1&0x3F]; D();
    L_CSUB: rd=bc[pc++]; rs1=bc[pc++]; rs2=bc[pc++]; vm->conf[rd&0x3F]-=vm->conf[rs1&0x3F]; D();
    L_CMUL: rd=bc[pc++]; rs1=bc[pc++]; rs2=bc[pc++]; vm->conf[rd&0x3F]*=vm->conf[rs1&0x3F]; D();
    L_CFUSE: rd=bc[pc++]; rs1=bc[pc++]; rs2=bc[pc++]; { int32_t cs=vm->conf[rs1&0x3F]+vm->conf[rs2&0x3F]; vm->conf[rd&0x3F]=cs?((get_reg(vm,rs1)*vm->conf[rs1&0x3F]+get_reg(vm,rs2)*vm->conf[rs2&0x3F])/cs):0; } D();
    
    /* Unknown opcode */
    D_BAD();
    
    #undef D
    #undef D_BAD
}
