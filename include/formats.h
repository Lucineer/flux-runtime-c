/* formats.h — FLUX FORMAT_A-G definitions and opcode constants
 *
 * Converged ISA from Oracle1 (115 opcodes) + JetsonClaw1 (128 opcodes)
 * ~200 defined opcodes across 7 format widths.
 *
 * FORMAT A: 1 byte  (system control, interrupts)
 * FORMAT B: 2 bytes (single register)
 * FORMAT C: 2 bytes (immediate8)
 * FORMAT D: 3 bytes (register + imm8)
 * FORMAT E: 4 bytes (3-register or reg + reg)
 * FORMAT F: 4 bytes (register + imm16)
 * FORMAT G: 5 bytes (register + register + imm16)
 */

#ifndef FLUX_FORMATS_H
#define FLUX_FORMATS_H

#define FLUX_NUM_REGS    64
#define FLUX_STACK_SIZE  256
#define FLUX_MEMORY_SIZE 65536  /* 64KB flat address space */

/* ── Format A: System Control (0x00-0x07) ── */
#define OP_HALT   0x00
#define OP_NOP    0x01
#define OP_RET    0x02
#define OP_IRET   0x03
#define OP_BRK    0x04
#define OP_WFI    0x05
#define OP_RESET  0x06
#define OP_SYN    0x07

/* ── Format B: Single Register (0x08-0x0F) ── */
#define OP_INC     0x08
#define OP_DEC     0x09
#define OP_NOT     0x0A
#define OP_NEG     0x0B
#define OP_PUSH    0x0C
#define OP_POP     0x0D
#define OP_CONF_LD 0x0E
#define OP_CONF_ST 0x0F

/* ── Format C: Immediate8 (0x10-0x17) ── */
#define OP_SYS     0x10
#define OP_TRAP    0x11
#define OP_DBG     0x12
#define OP_CLF     0x13
#define OP_SEMA    0x14
#define OP_YIELD   0x15
#define OP_CACHE   0x16
#define OP_STRIPCF 0x17

/* ── Format D: Register + Imm8 (0x18-0x1F) ── */
#define OP_MOVI  0x18
#define OP_ADDI  0x19
#define OP_SUBI  0x1A
#define OP_ANDI  0x1B
#define OP_ORI   0x1C
#define OP_XORI  0x1D
#define OP_SHLI  0x1E
#define OP_SHRI  0x1F

/* ── Format E: 3-Register Arithmetic (0x20-0x2F) ── */
#define OP_ADD    0x20
#define OP_SUB    0x21
#define OP_MUL    0x22
#define OP_DIV    0x23
#define OP_MOD    0x24
#define OP_AND    0x25
#define OP_OR     0x26
#define OP_XOR    0x27
#define OP_SHL    0x28
#define OP_SHR    0x29
#define OP_MIN    0x2A
#define OP_MAX    0x2B
#define OP_CMP_EQ 0x2C
#define OP_CMP_LT 0x2D
#define OP_CMP_GT 0x2E
#define OP_CMP_NE 0x2F

/* ── Format E: Float/Memory/Control (0x30-0x3F) ── */
#define OP_FADD  0x30
#define OP_FSUB  0x31
#define OP_FMUL  0x32
#define OP_FDIV  0x33
#define OP_FMIN  0x34
#define OP_FMAX  0x35
#define OP_FTOI  0x36
#define OP_ITOF  0x37
#define OP_LOAD  0x38
#define OP_STORE 0x39
#define OP_MOV   0x3A
#define OP_SWP   0x3B
#define OP_JZ    0x3C
#define OP_JNZ   0x3D
#define OP_JLT   0x3E
#define OP_JGT   0x3F

/* ── Format F: Register + Imm16 (0x40-0x4F) ── */
#define OP_MOVI16 0x40
#define OP_ADDI16 0x41
#define OP_SUBI16 0x42
#define OP_JMP    0x43
#define OP_JAL    0x44
#define OP_CALL   0x45
#define OP_LOOP   0x46
#define OP_JEQ    0x47
#define OP_JNE    0x48
#define OP_JLE    0x49
#define OP_JGE    0x4A
#define OP_LDI8   0x4B  /* rd = mem[imm16] */
#define OP_STI8   0x4C  /* mem[imm16] = rd */

/* ── Format G: Reg + Reg + Imm16 (0x50-0x5F) ── */
#define OP_LOADI  0x50  /* rd = mem[rs1 + imm16] */
#define OP_STOREI 0x51  /* mem[rs1 + imm16] = rd */

/* ── Format E: Agent-to-Agent (0x60-0x6F) ── */
#define OP_TELL      0x60
#define OP_ASK       0x61
#define OP_DELEGATE  0x62
#define OP_BROADCAST 0x63
#define OP_REDUCE    0x64
#define OP_REPLY     0x65
#define OP_FORWARD   0x66
#define OP_LISTEN    0x67
#define OP_FORK      0x68
#define OP_JOIN      0x69
#define OP_WAIT      0x6A
#define OP_SIGNAL    0x6B

/* ── Format E: Confidence-aware (0x70-0x7F) ── */
#define OP_CONF_ADD   0x70
#define OP_CONF_SUB   0x71
#define OP_CONF_MUL   0x72
#define OP_CONF_MERGE 0x73
#define OP_CONF_FUSE  0x74
#define OP_CONF_CHAIN 0x75
#define OP_CONF_SET   0x76
#define OP_CONF_GET   0x77
#define OP_CONF_CLAMP 0x78
#define OP_CONF_DECAY 0x79

/* ── Format E: Viewpoint (Babel, 0x80-0x8F) ── */
#define OP_VP_LOAD  0x80
#define OP_VP_STORE 0x81
#define OP_VP_SET   0x82
#define OP_VP_GET   0x83

/* ── Format E: Biology/Sensor (JetsonClaw1, 0x90-0x9F) ── */
#define OP_SENSE    0x90
#define OP_GPS      0x91
#define OP_ACCEL    0x92
#define OP_GYRO     0x93
#define OP_TEMP     0x94
#define OP_VOLT     0x95
#define OP_ATP_GEN  0x96
#define OP_ATP_USE  0x97
#define OP_APOPTOSIS 0x98

/* ── Format E: Extended Math (0xA0-0xAF) ── */
#define OP_ABS      0xA0
#define OP_SQRT     0xA1
#define OP_POW      0xA2
#define OP_LOG      0xA3
#define OP_RAND     0xA4
#define OP_CLAMP    0xA5
#define OP_LERP     0xA6
#define OP_HASH     0xA7

/* ── Format A: Extended System/Debug (0xF0-0xFF) ── */
#define OP_DUMP     0xF0
#define OP_PROFILE  0xF1
#define OP_WATCH    0xF2

#endif /* FLUX_FORMATS_H */
