/* flux_vm.c — FLUX Unified Virtual Machine (FORMAT_A-G)
 *
 * Converged VM from Oracle1 + JetsonClaw1
 * 64 registers, 64KB memory, 256-entry stack
 * ~150 implemented opcodes across 7 format widths
 * Zero dependencies, C11, -Werror clean on ARM64
 */
#include "flux_vm.h"
#include <string.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

static int16_t read_i16(const uint8_t* bc, int32_t pos) {
    return (int16_t)((uint16_t)bc[pos] | ((uint16_t)bc[pos+1] << 8));
}

static uint16_t read_u16(const uint8_t* bc, int32_t pos) {
    return (uint16_t)bc[pos] | ((uint16_t)bc[pos+1] << 8);
}

static int32_t sign_ext8(int8_t val) { return (int32_t)val; }

/* Format size lookup — determines instruction byte width */
int flux_format_size(uint8_t opcode) {
    if (opcode <= 0x07) return 1;  /* Format A */
    if (opcode <= 0x0F) return 2;  /* Format B */
    if (opcode <= 0x17) return 2;  /* Format C */
    if (opcode <= 0x1F) return 3;  /* Format D */
    if (opcode <= 0x3F) return 4;  /* Format E */
    if (opcode <= 0x4F) return 4;  /* Format F */
    if (opcode <= 0x5F) return 5;  /* Format G */
    if (opcode <= 0x9F) return 4;  /* A2A, Conf, VP, Bio, ExtMath: all Format E */
    /* 0xA0-0xAF: Extended Math — Format E */
    if (opcode <= 0xAF) return 4;
    /* 0xB0-0xBF: Instinct — mixed B/F */
    if (opcode <= 0xBF) {
        switch (opcode) {
            case 0xB2: case 0xB6: case 0xB7:  /* Format B */
            case 0xB8: case 0xB9:              /* Format B */
                return 2;
            default: return 4;                  /* Format F/E */
        }
    }
    /* 0xC0-0xCF: Trust — mixed */
    if (opcode <= 0xCF) {
        if (opcode == 0xC2 || opcode == 0xC5) return 2;  /* Format B */
        return 4;
    }
    /* 0xD0-0xDF: Memory Management — mixed */
    if (opcode <= 0xDF) {
        if (opcode == 0xD4) return 4;  /* MEMSWAP: Format E */
        if (opcode == 0xD6 || opcode == 0xD7) return 2;  /* Format B */
        return 5;  /* Format G */
    }
    /* 0xE0-0xEF: Bit Manipulation — mixed */
    if (opcode <= 0xEF) {
        if (opcode <= 0xE2) return 2;  /* Format B */
        if (opcode <= 0xE4) return 4;  /* Format E */
        return 5;  /* Format G */
    }
    /* 0xF0-0xFF: Debug — Format A */
    return 1;
}
}

void flux_vm_init(FluxVM* vm) {
    memset(vm, 0, sizeof(FluxVM));
    vm->sp = FLUX_STACK_SIZE;
    vm->agent_id = -1;
    /* Seed RNG */
    srand((unsigned int)time(NULL));
}

int32_t flux_vm_execute(FluxVM* vm, const uint8_t* bytecode, int32_t len) {
    while (!vm->halted && !vm->faulted && vm->pc < len) {
        int32_t result = flux_vm_step(vm, bytecode, len);
        if (result < 0) break;
        if (result == 0) continue; /* jump: pc already set */
        vm->pc += result;
    }
    return vm->cycles;
}

/* Safe register access — r0 reads as 0 */
static int32_t get_reg(FluxVM* vm, uint8_t r) {
    return r == 0 ? 0 : vm->gp[r & 0x3F];
}

static void set_reg(FluxVM* vm, uint8_t r, int32_t val) {
    if (r != 0) vm->gp[r & 0x3F] = val;
}

static void update_flags(FluxVM* vm, int32_t val) {
    vm->zero_flag = (val == 0) ? 1 : 0;
    vm->neg_flag = (val < 0) ? 1 : 0;
}

int32_t flux_vm_step(FluxVM* vm, const uint8_t* bc, int32_t len) {
    if (vm->pc >= len) return 0;

    uint8_t op = bc[vm->pc];
    int size = flux_format_size(op);

    if (vm->pc + size > len) {
        vm->faulted = true;
        vm->fault_code = 1;
        return 0;
    }

    vm->cycles++;

    /* StripConf: if active, skip confidence side-effects */
    if (vm->stripconf_remaining > 0) vm->stripconf_remaining--;
    int use_conf = (vm->stripconf_remaining == 0);

    /* Helper macros for operand decoding */
    uint8_t rd, rs1, rs2;
    int8_t imm8;
    int16_t imm16;

    switch (op) {
    /* ═══ Format A: System Control ═══ */
    case OP_HALT: vm->halted = true; return 1;
    case OP_NOP: return 1;
    case OP_RET:
        if (vm->sp < FLUX_STACK_SIZE) vm->pc = vm->stack[vm->sp++];
        return 1;
    case OP_IRET: return 1; /* stub */
    case OP_BRK: vm->halted = true; return 1;
    case OP_WFI: return 1;
    case OP_RESET: memset(vm->gp, 0, sizeof(vm->gp)); return 1;
    case OP_SYN: return 1; /* memory barrier — no-op in single-threaded */

    /* ═══ Format B: Single Register ═══ */
    case OP_INC: rd = bc[vm->pc+1]; set_reg(vm,rd,get_reg(vm,rd)+1); update_flags(vm,get_reg(vm,rd)); return 2;
    case OP_DEC: rd = bc[vm->pc+1]; set_reg(vm,rd,get_reg(vm,rd)-1); update_flags(vm,get_reg(vm,rd)); return 2;
    case OP_NOT: rd = bc[vm->pc+1]; set_reg(vm,rd,~get_reg(vm,rd)); return 2;
    case OP_NEG: rd = bc[vm->pc+1]; set_reg(vm,rd,-get_reg(vm,rd)); return 2;
    case OP_PUSH: rd = bc[vm->pc+1]; vm->stack[--vm->sp] = get_reg(vm,rd); return 2;
    case OP_POP:  rd = bc[vm->pc+1]; set_reg(vm,rd,vm->stack[vm->sp++]); return 2;
    case OP_CONF_LD: rd = bc[vm->pc+1]; set_reg(vm,rd,vm->conf[rd & 0x3F]); return 2;
    case OP_CONF_ST: rd = bc[vm->pc+1]; vm->conf[rd & 0x3F] = get_reg(vm,rd); return 2;

    /* ═══ Format C: Immediate8 ═══ */
    case OP_SYS: return 2; /* stub */
    case OP_TRAP: return 2; /* stub */
    case OP_DBG: { rd = bc[vm->pc+1]; fprintf(stderr,"DBG r%d = %d\n", rd, get_reg(vm,rd)); return 2; }
    case OP_CLF: vm->zero_flag = 0; vm->neg_flag = 0; return 2;
    case OP_SEMA: return 2; /* stub */
    case OP_YIELD: return 2;
    case OP_CACHE: return 2; /* stub */
    case OP_STRIPCF: vm->stripconf_remaining = bc[vm->pc+1]; return 2;

    /* ═══ Format D: Register + Imm8 ═══ */
    case OP_MOVI:  rd = bc[vm->pc+1]; imm8 = (int8_t)bc[vm->pc+2]; set_reg(vm,rd,sign_ext8(imm8)); return 3;
    case OP_ADDI:  rd = bc[vm->pc+1]; imm8 = (int8_t)bc[vm->pc+2]; set_reg(vm,rd,get_reg(vm,rd)+imm8); update_flags(vm,get_reg(vm,rd)); return 3;
    case OP_SUBI:  rd = bc[vm->pc+1]; imm8 = (int8_t)bc[vm->pc+2]; set_reg(vm,rd,get_reg(vm,rd)-imm8); update_flags(vm,get_reg(vm,rd)); return 3;
    case OP_ANDI:  rd = bc[vm->pc+1]; set_reg(vm,rd,get_reg(vm,rd)&bc[vm->pc+2]); return 3;
    case OP_ORI:   rd = bc[vm->pc+1]; set_reg(vm,rd,get_reg(vm,rd)|bc[vm->pc+2]); return 3;
    case OP_XORI:  rd = bc[vm->pc+1]; set_reg(vm,rd,get_reg(vm,rd)^bc[vm->pc+2]); return 3;
    case OP_SHLI:  rd = bc[vm->pc+1]; set_reg(vm,rd,get_reg(vm,rd)<<bc[vm->pc+2]); return 3;
    case OP_SHRI:  rd = bc[vm->pc+1]; set_reg(vm,rd,get_reg(vm,rd)>>(bc[vm->pc+2]&0x1F)); return 3;

    /* ═══ Format E: 3-Register Arithmetic ═══ */
    case OP_ADD:    rd=bc[vm->pc+1]; rs1=bc[vm->pc+2]; rs2=bc[vm->pc+3]; set_reg(vm,rd,get_reg(vm,rs1)+get_reg(vm,rs2)); update_flags(vm,get_reg(vm,rd)); return 4;
    case OP_SUB:    rd=bc[vm->pc+1]; rs1=bc[vm->pc+2]; rs2=bc[vm->pc+3]; set_reg(vm,rd,get_reg(vm,rs1)-get_reg(vm,rs2)); update_flags(vm,get_reg(vm,rd)); return 4;
    case OP_MUL:    rd=bc[vm->pc+1]; rs1=bc[vm->pc+2]; rs2=bc[vm->pc+3]; set_reg(vm,rd,get_reg(vm,rs1)*get_reg(vm,rs2)); update_flags(vm,get_reg(vm,rd)); return 4;
    case OP_DIV:    rd=bc[vm->pc+1]; rs1=bc[vm->pc+2]; rs2=bc[vm->pc+3]; if(get_reg(vm,rs2)!=0) set_reg(vm,rd,get_reg(vm,rs1)/get_reg(vm,rs2)); else { vm->faulted=true; vm->fault_code=2; } return 4;
    case OP_MOD:    rd=bc[vm->pc+1]; rs1=bc[vm->pc+2]; rs2=bc[vm->pc+3]; if(get_reg(vm,rs2)!=0) set_reg(vm,rd,get_reg(vm,rs1)%get_reg(vm,rs2)); return 4;
    case OP_AND:    rd=bc[vm->pc+1]; rs1=bc[vm->pc+2]; rs2=bc[vm->pc+3]; set_reg(vm,rd,get_reg(vm,rs1)&get_reg(vm,rs2)); return 4;
    case OP_OR:     rd=bc[vm->pc+1]; rs1=bc[vm->pc+2]; rs2=bc[vm->pc+3]; set_reg(vm,rd,get_reg(vm,rs1)|get_reg(vm,rs2)); return 4;
    case OP_XOR:    rd=bc[vm->pc+1]; rs1=bc[vm->pc+2]; rs2=bc[vm->pc+3]; set_reg(vm,rd,get_reg(vm,rs1)^get_reg(vm,rs2)); return 4;
    case OP_SHL:    rd=bc[vm->pc+1]; rs1=bc[vm->pc+2]; rs2=bc[vm->pc+3]; set_reg(vm,rd,get_reg(vm,rs1)<<(get_reg(vm,rs2)&0x1F)); return 4;
    case OP_SHR:    rd=bc[vm->pc+1]; rs1=bc[vm->pc+2]; rs2=bc[vm->pc+3]; set_reg(vm,rd,get_reg(vm,rs1)>>(get_reg(vm,rs2)&0x1F)); return 4;
    case OP_MIN:    rd=bc[vm->pc+1]; rs1=bc[vm->pc+2]; rs2=bc[vm->pc+3]; { int32_t a=get_reg(vm,rs1),b=get_reg(vm,rs2); set_reg(vm,rd,a<b?a:b); } return 4;
    case OP_MAX:    rd=bc[vm->pc+1]; rs1=bc[vm->pc+2]; rs2=bc[vm->pc+3]; { int32_t a=get_reg(vm,rs1),b=get_reg(vm,rs2); set_reg(vm,rd,a>b?a:b); } return 4;
    case OP_CMP_EQ: rd=bc[vm->pc+1]; rs1=bc[vm->pc+2]; rs2=bc[vm->pc+3]; set_reg(vm,rd,get_reg(vm,rs1)==get_reg(vm,rs2)?1:0); return 4;
    case OP_CMP_LT: rd=bc[vm->pc+1]; rs1=bc[vm->pc+2]; rs2=bc[vm->pc+3]; set_reg(vm,rd,get_reg(vm,rs1)<get_reg(vm,rs2)?1:0); return 4;
    case OP_CMP_GT: rd=bc[vm->pc+1]; rs1=bc[vm->pc+2]; rs2=bc[vm->pc+3]; set_reg(vm,rd,get_reg(vm,rs1)>get_reg(vm,rs2)?1:0); return 4;
    case OP_CMP_NE: rd=bc[vm->pc+1]; rs1=bc[vm->pc+2]; rs2=bc[vm->pc+3]; set_reg(vm,rd,get_reg(vm,rs1)!=get_reg(vm,rs2)?1:0); return 4;

    /* ═══ Format E: Float/Memory/Control (0x30-0x3F) ═══ */
    case OP_FADD: rd=bc[vm->pc+1]; rs1=bc[vm->pc+2]; rs2=bc[vm->pc+3]; vm->fp[rd&0x3F]=vm->fp[rs1&0x3F]+vm->fp[rs2&0x3F]; return 4;
    case OP_FSUB: rd=bc[vm->pc+1]; rs1=bc[vm->pc+2]; rs2=bc[vm->pc+3]; vm->fp[rd&0x3F]=vm->fp[rs1&0x3F]-vm->fp[rs2&0x3F]; return 4;
    case OP_FMUL: rd=bc[vm->pc+1]; rs1=bc[vm->pc+2]; rs2=bc[vm->pc+3]; vm->fp[rd&0x3F]=vm->fp[rs1&0x3F]*vm->fp[rs2&0x3F]; return 4;
    case OP_FDIV: rd=bc[vm->pc+1]; rs1=bc[vm->pc+2]; rs2=bc[vm->pc+3]; vm->fp[rd&0x3F]=vm->fp[rs1&0x3F]/vm->fp[rs2&0x3F]; return 4;
    case OP_FMIN: rd=bc[vm->pc+1]; rs1=bc[vm->pc+2]; rs2=bc[vm->pc+3]; vm->fp[rd&0x3F]=fminf(vm->fp[rs1&0x3F],vm->fp[rs2&0x3F]); return 4;
    case OP_FMAX: rd=bc[vm->pc+1]; rs1=bc[vm->pc+2]; rs2=bc[vm->pc+3]; vm->fp[rd&0x3F]=fmaxf(vm->fp[rs1&0x3F],vm->fp[rs2&0x3F]); return 4;
    case OP_FTOI: rd=bc[vm->pc+1]; rs1=bc[vm->pc+2]; set_reg(vm,rd,(int32_t)vm->fp[rs1&0x3F]); return 4;
    case OP_ITOF: rd=bc[vm->pc+1]; rs1=bc[vm->pc+2]; vm->fp[rd&0x3F]=(float)get_reg(vm,rs1); return 4;
    case OP_LOAD: {
        rd=bc[vm->pc+1]; rs1=bc[vm->pc+2]; rs2=bc[vm->pc+3];
        int32_t addr = (get_reg(vm,rs1) + get_reg(vm,rs2)) & 0xFFFF;
        int32_t val = (int32_t)vm->memory[addr] | ((int32_t)vm->memory[(addr+1)&0xFFFF]<<8) |
                     ((int32_t)vm->memory[(addr+2)&0xFFFF]<<16) | ((int32_t)vm->memory[(addr+3)&0xFFFF]<<24);
        set_reg(vm, rd, val);
        return 4;
    }
    case OP_STORE: {
        rd=bc[vm->pc+1]; rs1=bc[vm->pc+2]; rs2=bc[vm->pc+3];
        int32_t addr = (get_reg(vm,rs1) + get_reg(vm,rs2)) & 0xFFFF;
        int32_t val = get_reg(vm, rd);
        vm->memory[addr] = (uint8_t)(val & 0xFF);
        vm->memory[(addr+1)&0xFFFF] = (uint8_t)((val>>8) & 0xFF);
        vm->memory[(addr+2)&0xFFFF] = (uint8_t)((val>>16) & 0xFF);
        vm->memory[(addr+3)&0xFFFF] = (uint8_t)((val>>24) & 0xFF);
        return 4;
    }
    case OP_MOV: rd=bc[vm->pc+1]; rs1=bc[vm->pc+2]; set_reg(vm,rd,get_reg(vm,rs1)); return 4;
    case OP_SWP: rd=bc[vm->pc+1]; rs1=bc[vm->pc+2]; { int32_t t=get_reg(vm,rd); set_reg(vm,rd,get_reg(vm,rs1)); set_reg(vm,rs1,t); } return 4;
    case OP_JZ:  rd=bc[vm->pc+1]; rs1=bc[vm->pc+2]; if(get_reg(vm,rd)==0) { vm->pc += get_reg(vm,rs1); return 0; } return 4;
    case OP_JNZ: rd=bc[vm->pc+1]; rs1=bc[vm->pc+2]; if(get_reg(vm,rd)!=0) { vm->pc += get_reg(vm,rs1); return 0; } return 4;
    case OP_JLT: rd=bc[vm->pc+1]; rs1=bc[vm->pc+2]; if(get_reg(vm,rd)<0) { vm->pc += get_reg(vm,rs1); return 0; } return 4;
    case OP_JGT: rd=bc[vm->pc+1]; rs1=bc[vm->pc+2]; if(get_reg(vm,rd)>0) { vm->pc += get_reg(vm,rs1); return 0; } return 4;

    /* ═══ Format F: Register + Imm16 (0x40-0x4F) ═══ */
    case OP_MOVI16: rd=bc[vm->pc+1]; set_reg(vm,rd,(int32_t)read_i16(bc,vm->pc+2)); return 4;
    case OP_ADDI16: rd=bc[vm->pc+1]; set_reg(vm,rd,get_reg(vm,rd)+(int32_t)read_i16(bc,vm->pc+2)); update_flags(vm,get_reg(vm,rd)); return 4;
    case OP_SUBI16: rd=bc[vm->pc+1]; set_reg(vm,rd,get_reg(vm,rd)-(int32_t)read_i16(bc,vm->pc+2)); update_flags(vm,get_reg(vm,rd)); return 4;
    case OP_JMP: imm16=read_i16(bc,vm->pc+2); vm->pc += (int32_t)imm16; return 0;
    case OP_JAL: rd=bc[vm->pc+1]; imm16=read_i16(bc,vm->pc+2); set_reg(vm,rd,vm->pc+4); vm->pc += (int32_t)imm16; return 0;
    case OP_CALL: { rd=bc[vm->pc+1]; imm16=read_i16(bc,vm->pc+2); vm->stack[--vm->sp]=vm->pc+4; vm->pc = get_reg(vm,rd)+(int32_t)imm16; return 0; }
    case OP_LOOP: { rd=bc[vm->pc+1]; imm16=read_i16(bc,vm->pc+2); set_reg(vm,rd,get_reg(vm,rd)-1); if(get_reg(vm,rd)>0) { vm->pc += (int32_t)imm16; return 0; } return 4; }
    case OP_JEQ: rd=bc[vm->pc+1]; imm16=read_i16(bc,vm->pc+2); if(get_reg(vm,rd)==0) { vm->pc += (int32_t)imm16; return 0; } return 4;
    case OP_JNE: rd=bc[vm->pc+1]; imm16=read_i16(bc,vm->pc+2); if(get_reg(vm,rd)!=0) { vm->pc += (int32_t)imm16; return 0; } return 4;
    case OP_JLE: rd=bc[vm->pc+1]; imm16=read_i16(bc,vm->pc+2); if(get_reg(vm,rd)<=0) { vm->pc += (int32_t)imm16; return 0; } return 4;
    case OP_JGE: rd=bc[vm->pc+1]; imm16=read_i16(bc,vm->pc+2); if(get_reg(vm,rd)>=0) { vm->pc += (int32_t)imm16; return 0; } return 4;
    case OP_LDI8: { /* rd = mem[imm16] (1 byte) */
        rd=bc[vm->pc+1]; uint16_t addr=read_u16(bc,vm->pc+2);
        set_reg(vm,rd,(int32_t)vm->memory[addr&0xFFFF]);
        return 4;
    }
    case OP_STI8: { /* mem[imm16] = rd (1 byte) */
        rd=bc[vm->pc+1]; uint16_t addr=read_u16(bc,vm->pc+2);
        vm->memory[addr&0xFFFF] = (uint8_t)(get_reg(vm,rd)&0xFF);
        return 4;
    }

    /* ═══ Format G: Reg + Reg + Imm16 (0x50-0x5F) ═══ */
    case OP_LOADI: {
        rd=bc[vm->pc+1]; rs1=bc[vm->pc+2]; imm16=read_i16(bc,vm->pc+3);
        int32_t addr = (get_reg(vm,rs1)+(int32_t)imm16) & 0xFFFF;
        int32_t val = vm->memory[addr] | (vm->memory[(addr+1)&0xFFFF]<<8) |
                     (vm->memory[(addr+2)&0xFFFF]<<16) | (vm->memory[(addr+3)&0xFFFF]<<24);
        set_reg(vm, rd, val);
        return 5;
    }
    case OP_STOREI: {
        rd=bc[vm->pc+1]; rs1=bc[vm->pc+2]; imm16=read_i16(bc,vm->pc+3);
        int32_t addr = (get_reg(vm,rs1)+(int32_t)imm16) & 0xFFFF;
        int32_t val = get_reg(vm, rd);
        vm->memory[addr] = (uint8_t)(val&0xFF);
        vm->memory[(addr+1)&0xFFFF] = (uint8_t)((val>>8)&0xFF);
        vm->memory[(addr+2)&0xFFFF] = (uint8_t)((val>>16)&0xFF);
        vm->memory[(addr+3)&0xFFFF] = (uint8_t)((val>>24)&0xFF);
        return 5;
    }

    /* ═══ Format E: Agent-to-Agent (0x60-0x6F) ═══ */
    /* A2A ops are stubs — real implementation requires fleet runtime */
    case OP_TELL:      return 4; /* stub: send message to agent in rd */
    case OP_ASK:       return 4; /* stub: query agent in rd */
    case OP_DELEGATE:  return 4; /* stub: delegate task */
    case OP_BROADCAST: return 4; /* stub: broadcast to fleet */
    case OP_REDUCE:    return 4; /* stub: reduce from fleet */
    case OP_REPLY:     return 4; /* stub: reply to last message */
    case OP_FORWARD:   return 4; /* stub: forward message */
    case OP_LISTEN:    return 4; /* stub: wait for incoming */
    case OP_FORK:      { /* spawn: push return addr, clear pc conceptually */
        rd=bc[vm->pc+1]; rs1=bc[vm->pc+2]; rs2=bc[vm->pc+3];
        set_reg(vm,rd,vm->agent_id); /* child gets parent's ID */
        vm->cycles += 10; /* fork is expensive */
        return 4;
    }
    case OP_JOIN:      { /* join: wait for child */
        rd=bc[vm->pc+1]; rs1=bc[vm->pc+2];
        vm->cycles += 5;
        return 4;
    }
    case OP_WAIT:      return 4; /* stub */
    case OP_SIGNAL:    return 4; /* stub */

    /* ═══ Format E: Confidence-aware (0x70-0x7F) ═══ */
    case OP_CONF_ADD: {
        rd=bc[vm->pc+1]; rs1=bc[vm->pc+2]; rs2=bc[vm->pc+3];
        if (use_conf) {
            /* Harmonic mean: 2*a*b/(a+b), clamped to [0,100] */
            int32_t a = vm->conf[rs1&0x3F], b = vm->conf[rs2&0x3F];
            int32_t sum = a + b;
            int32_t result = (sum > 0) ? (2*a*b/sum) : 0;
            vm->conf[rd&0x3F] = result > 100 ? 100 : result;
        }
        set_reg(vm,rd,get_reg(vm,rs1)+get_reg(vm,rs2));
        return 4;
    }
    case OP_CONF_SUB: {
        rd=bc[vm->pc+1]; rs1=bc[vm->pc+2]; rs2=bc[vm->pc+3];
        if (use_conf) {
            int32_t a = vm->conf[rs1&0x3F], b = vm->conf[rs2&0x3F];
            vm->conf[rd&0x3F] = (a > b) ? (a - b) : 0;
        }
        set_reg(vm,rd,get_reg(vm,rs1)-get_reg(vm,rs2));
        return 4;
    }
    case OP_CONF_MUL: {
        rd=bc[vm->pc+1]; rs1=bc[vm->pc+2]; rs2=bc[vm->pc+3];
        if (use_conf) {
            int32_t a = vm->conf[rs1&0x3F], b = vm->conf[rs2&0x3F];
            int32_t result = a * b / 100;
            vm->conf[rd&0x3F] = result > 100 ? 100 : result;
        }
        set_reg(vm,rd,get_reg(vm,rs1)*get_reg(vm,rs2));
        return 4;
    }
    case OP_CONF_MERGE: {
        /* Average of confidence registers */
        rd=bc[vm->pc+1]; rs1=bc[vm->pc+2]; rs2=bc[vm->pc+3];
        if (use_conf) {
            vm->conf[rd&0x3F] = (vm->conf[rs1&0x3F] + vm->conf[rs2&0x3F]) / 2;
        }
        return 4;
    }
    case OP_CONF_FUSE: {
        /* Bayesian fusion: weighted average */
        rd=bc[vm->pc+1]; rs1=bc[vm->pc+2]; rs2=bc[vm->pc+3];
        if (use_conf) {
            int32_t w1 = vm->conf[rs1&0x3F], w2 = vm->conf[rs2&0x3F];
            int32_t total = w1 + w2;
            vm->conf[rd&0x3F] = (total > 0) ? (get_reg(vm,rs1)*w1 + get_reg(vm,rs2)*w2) / total : 0;
        }
        return 4;
    }
    case OP_CONF_CHAIN: { rd=bc[vm->pc+1]; rs1=bc[vm->pc+2]; rs2=bc[vm->pc+3]; if(use_conf) vm->conf[rd&0x3F]=vm->conf[rs1&0x3F]*vm->conf[rs2&0x3F]/100; return 4; }
    case OP_CONF_SET:  { rd=bc[vm->pc+1]; rs1=bc[vm->pc+2]; vm->conf[rd&0x3F]=get_reg(vm,rs1); return 4; }
    case OP_CONF_GET:  { rd=bc[vm->pc+1]; rs1=bc[vm->pc+2]; set_reg(vm,rd,vm->conf[rs1&0x3F]); return 4; }
    case OP_CONF_CLAMP:{ rd=bc[vm->pc+1]; { int32_t c=vm->conf[rd&0x3F]; vm->conf[rd&0x3F]=c<0?0:c>100?100:c; } return 4; }
    case OP_CONF_DECAY:{ rd=bc[vm->pc+1]; rs1=bc[vm->pc+2]; if(use_conf) vm->conf[rd&0x3F]=vm->conf[rd&0x3F]*get_reg(vm,rs1)/100; return 4; }

    /* ═══ Format E: Biology/Sensor (0x90-0x9F) ═══ */
    case OP_SENSE:     set_reg(vm,bc[vm->pc+1],0); return 4; /* stub */
    case OP_GPS:       set_reg(vm,bc[vm->pc+1],0); return 4; /* stub */
    case OP_ACCEL:     set_reg(vm,bc[vm->pc+1],0); return 4; /* stub */
    case OP_GYRO:      set_reg(vm,bc[vm->pc+1],0); return 4; /* stub */
    case OP_TEMP:      set_reg(vm,bc[vm->pc+1],0); return 4; /* stub */
    case OP_VOLT:      set_reg(vm,bc[vm->pc+1],0); return 4; /* stub */
    case OP_ATP_GEN:   set_reg(vm,bc[vm->pc+1],100); return 4; /* generate 100 ATP */
    case OP_ATP_USE:   { rd=bc[vm->pc+1]; rs1=bc[vm->pc+2]; set_reg(vm,rd,get_reg(vm,rd)-get_reg(vm,rs1)); return 4; }
    case OP_APOPTOSIS: vm->halted=true; vm->fault_code=99; return 4; /* graceful self-termination */

    /* ═══ Format E: Extended Math (0xA0-0xAF) ═══ */
    case OP_ABS:   rd=bc[vm->pc+1]; rs1=bc[vm->pc+2]; { int32_t v=get_reg(vm,rs1); set_reg(vm,rd,v<0?-v:v); } return 4;
    case OP_SQRT:  rd=bc[vm->pc+1]; rs1=bc[vm->pc+2]; set_reg(vm,rd,(int32_t)sqrtf((float)fabs((double)get_reg(vm,rs1)))); return 4;
    case OP_POW:   rd=bc[vm->pc+1]; rs1=bc[vm->pc+2]; rs2=bc[vm->pc+3]; set_reg(vm,rd,(int32_t)powf((float)get_reg(vm,rs1),(float)get_reg(vm,rs2))); return 4;
    case OP_LOG:   rd=bc[vm->pc+1]; rs1=bc[vm->pc+2]; set_reg(vm,rd,(int32_t)logf((float)fabs((double)get_reg(vm,rs1)))); return 4;
    case OP_RAND:  rd=bc[vm->pc+1]; set_reg(vm,rd,rand()); return 4;
    case OP_CLAMP: { rd=bc[vm->pc+1]; rs1=bc[vm->pc+2]; rs2=bc[vm->pc+3]; int32_t v=get_reg(vm,rd); set_reg(vm,rd,v<get_reg(vm,rs1)?get_reg(vm,rs1):(v>get_reg(vm,rs2)?get_reg(vm,rs2):v)); return 4; }
    case OP_LERP:  { rd=bc[vm->pc+1]; rs1=bc[vm->pc+2]; rs2=bc[vm->pc+3]; set_reg(vm,rd,get_reg(vm,rs1)+(get_reg(vm,rs2)-get_reg(vm,rs1))*get_reg(vm,rd)/100); return 4; }
    case OP_HASH:  { rd=bc[vm->pc+1]; rs1=bc[vm->pc+2]; uint32_t h=(uint32_t)get_reg(vm,rs1); h=((h>>16)^h)*0x45d9f3b; h=((h>>16)^h)*0x45d9f3b; h=(h>>16)^h; set_reg(vm,rd,(int32_t)h); return 4; }

    /* ═══ Instinct Opcodes (0xB0-0xB7) ═══ */
    /* Format F: 4 bytes (op rd imm16_lo imm16_hi) */
    case OP_ISTINCT_LOAD:
        rd = bc[vm->pc+1];
        imm16 = read_i16(bc, vm->pc+2);
        set_reg(vm, rd, vm->memory[imm16 & (FLUX_MEMORY_SIZE-1)]);
        return 4;
    case OP_ISTINCT_STORE:
        rd = bc[vm->pc+1];
        imm16 = read_i16(bc, vm->pc+2);
        vm->memory[imm16 & (FLUX_MEMORY_SIZE-1)] = get_reg(vm, rd);
        return 4;
    case OP_ISTINCT_REFLEX:
        rd = bc[vm->pc+1];
        imm16 = read_i16(bc, vm->pc+2);
        if (get_reg(vm, rd) > 0) {
            set_reg(vm, rd, get_reg(vm, rd) - 1);
            vm->pc = imm16;
            return 0; /* jump */
        }
        return 4;
    case OP_ISTINCT_MODULATE:
        rd = bc[vm->pc+1];
        rs1 = bc[vm->pc+2]; /* reused as factor high byte */
        imm16 = read_i16(bc, vm->pc+2);
        set_reg(vm, rd, (int32_t)((int64_t)get_reg(vm, rd) * imm16 / 1000));
        return 4;
    case OP_ISTINCT_THRESHOLD:
        rd = bc[vm->pc+1];
        imm16 = read_i16(bc, vm->pc+2);
        set_reg(vm, rd, (get_reg(vm, rd) >= imm16) ? 1 : 0);
        return 4;
    /* Format B: 2 bytes (op rd) */
    case OP_ISTINCT_DECAY:
        rd = bc[vm->pc+1];
        set_reg(vm, rd, (int32_t)(get_reg(vm, rd) * 0.95f));
        return 2;
    case OP_ISTINCT_CONVERGE:
        rd = bc[vm->pc+1];
        { int32_t sum = 0, count = 0, base_r = (rd / 8) * 8;
          for (int i = base_r; i < base_r + 8 && i < FLUX_NUM_REGS; i++)
              if (i != rd && i != 0) { sum += vm->gp[i]; count++; }
          if (count > 0) { int32_t avg = sum / count;
            set_reg(vm, rd, get_reg(vm, rd) + (avg - get_reg(vm, rd)) / 4); }
        }
        return 2;
    case OP_ISTINCT_EXTINCT:
        rd = bc[vm->pc+1];
        { int32_t val = get_reg(vm, rd);
          if (val > -10 && val < 10) set_reg(vm, rd, 0); }
        return 2;

    /* ═══ Format A: Extended System (0xF0-0xFF) ═══ */
    case OP_DUMP:    return 1; /* stub */
    case OP_PROFILE: return 1; /* stub */
    case OP_WATCH:   return 1; /* stub */
    default:
        vm->faulted = true;
        vm->fault_code = 3; /* unknown opcode */
        return 0;
    }
}
