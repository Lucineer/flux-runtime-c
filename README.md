# flux-runtime-c

Converged FLUX Virtual Machine -- C11 runtime with FORMAT_A-G instruction encoding.

Joint work: Oracle1 (SuperInstance) + JetsonClaw1 (Lucineer).

## Architecture

| Feature | Value |
|---------|-------|
| Registers | 64 GP + 64 Confidence + 64 Float |
| Memory | 64KB flat address space |
| Stack | 256 entries |
| Opcodes | ~150 defined (7 format widths) |
| Dispatch | Switch statement |
| Dependencies | Zero (C11 + libm) |


## Complete ISA — 200 Opcodes

| Range | Category | Opcodes |
|-------|----------|---------|
| 0x00-0x07 | System | HALT NOP RET IRET BRK WFI RESET SYN |
| 0x08-0x0F | Register | INC DEC NOT NEG PUSH_R POP_R CONF_LD CONF_ST |
| 0x10-0x17 | Control | SYS TRAP DBG CLF SEMA YIELD CACHE STRIPCF |
| 0x18-0x1F | Imm8 | MOVI ADDI SUBI ANDI ORI XORI SHLI SHRI |
| 0x20-0x3F | Core | ADD SUB MUL DIV MOD LOAD STORE MOV SWP CMP JZ JNZ JLT JGT FADD FSUB FMUL FDIV FMIN FMAX FTOF ITOR |
| 0x40-0x4F | Imm16 | MOVI16 JMP JAL CALL LOOP JEQ JNE JLE JGE LDI8 STI8 |
| 0x50-0x5F | Indexed | LOADI STOREI |
| 0x60-0x6F | A2A | TELL ASK DELEGATE BROADCAST REDUCE REPLY FORWARD LISTEN FORK JOIN WAIT SIGNAL |
| 0x70-0x7F | Confidence | CONF_ADD SUB MUL MERGE FUSE CHAIN SET GET CLAMP DECAY |
| 0x80-0x8F | Viewpoint | VP_LOAD VP_STORE VP_SET VP_GET |
| 0x90-0x9F | Biology | SENSE GPS ACCEL GYRO TEMP VOLT ATP_GEN ATP_USE APOPTOSIS |
| 0xA0-0xAF | ExtMath | ABS SQRT POW LOG RAND CLAMP LERP HASH |
| 0xB0-0xBF | Instinct | LOAD STORE DECAY REFLEX MODULATE THRESHOLD CONVERGE EXTINCT HABITUATE SENSITIZE GENERALIZE SPECIALIZE DIFFUSE INHIBIT SUM DIFF |
| 0xC0-0xCF | Trust | INIT UPDATE DECAY COMPARE MIN REVOKE RESTRICT SCORE AVERAGE BOOST WEAKEN VERIFY TRANSFER SEED SCOPE FLOOR |
| 0xD0-0xDF | Memory | MEMSET MEMCPY MEMCMP MEMFILL MEMSWAP MEMSCAN MEMINDEX MEMSUM RANGE_MIN RANGE_MAX STACKFRAME HEAP_ALLOC HEAP_FREE |
| 0xE0-0xEF | BitTime | BITCOUNT BITSCAN BITREV ROTL ROTR BEXTR BDEP MERGE RAND_RANGE RAND_WEIGHTED CYCLE_READ ALARM CRC32 ENCODE_VARINT DECODE_VARINT ATOMIC_CAS |
| 0xF0-0xFF | Debug | DUMP PROFILE WATCH |

## Dispatch Variants

| Variant | Speed (ARM64) | Notes |
|---------|--------------|-------|
| Switch | 362 Mops/s | Baseline, portable |
| Computed Goto | 1,564 Mops/s | 4.3x, exploits RAS |
| Token Threaded | 303 Mops/s | Direct threading w/o labels |
| Subroutine | 166 Mops/s | Call/ret dispatch |
| Coroutine | pending | setjmp/longjmp (ARM64 issues) |

## Test Results

92 tests, 0 failures — ARM64 Jetson Orin Nano, gcc 11.4, -O2.
## Instruction Formats

| Format | Width | Encoding | Examples |
|--------|-------|----------|---------|
| A | 1 byte | opcode | HALT, NOP, SYN |
| B | 2 bytes | opcode + rd | INC, DEC, PUSH, POP |
| C | 2 bytes | opcode + imm8 | SYS, TRAP, DBG, STRIPCF |
| D | 3 bytes | opcode + rd + imm8 | MOVI, ADDI, SUBI, ANDI |
| E | 4 bytes | opcode + rd + rs1 + rs2 | ADD, SUB, MUL, LOAD, STORE |
| F | 4 bytes | opcode + rd + imm16 | MOVI16, JMP, JAL, CALL, LOOP |
| G | 5 bytes | opcode + rd + rs1 + imm16 | LOADI, STOREI |

## Opcode Categories

- **System Control (0x00-0x07)**: HALT, NOP, RET, IRET, BRK, WFI, RESET, SYN
- **Single Register (0x08-0x0F)**: INC, DEC, NOT, NEG, PUSH, POP, CONF_LD, CONF_ST
- **Immediate (0x10-0x1F)**: SYS, TRAP, DBG, CLF, STRIPCF, MOVI, ADDI, SUBI
- **Arithmetic (0x20-0x2F)**: ADD, SUB, MUL, DIV, MOD, AND, OR, XOR, SHL, SHR, MIN, MAX
- **Comparison (0x2C-0x2F)**: CMP_EQ, CMP_LT, CMP_GT, CMP_NE
- **Float (0x30-0x37)**: FADD, FSUB, FMUL, FDIV, FMIN, FMAX, FTOI, ITOF
- **Memory (0x38-0x51)**: LOAD, STORE, LOADI, STOREI, LDI8, STI8
- **Control Flow (0x3A-0x4A)**: MOV, SWP, JZ, JNZ, JMP, JAL, CALL, LOOP, JEQ, JNE
- **Agent-to-Agent (0x60-0x6B)**: TELL, ASK, DELEGATE, BROADCAST, REDUCE, REPLY, FORWARD, LISTEN, FORK, JOIN, WAIT, SIGNAL
- **Confidence (0x70-0x79)**: CONF_ADD, CONF_SUB, CONF_MUL, CONF_MERGE, CONF_FUSE, CONF_CHAIN, CONF_SET, CONF_GET, CONF_CLAMP, CONF_DECAY
- **Viewpoint/Babel (0x80-0x83)**: VP_LOAD, VP_STORE, VP_SET, VP_GET
- **Biology/Sensor (0x90-0x98)**: SENSE, GPS, ACCEL, GYRO, TEMP, VOLT, ATP_GEN, ATP_USE, APOPTOSIS
- **Extended Math (0xA0-0xA7)**: ABS, SQRT, POW, LOG, RAND, CLAMP, LERP, HASH
- **Debug (0xF0-0xF2)**: DUMP, PROFILE, WATCH

## Confidence System (Think Tank Decision)

Confidence propagation is **OPTIONAL** (not default). Use STRIPCF to skip confidence side-effects for N instructions. Without STRIPCF, CONF_ ops propagate through the confidence register file:

- **Harmonic Mean** (CONF_ADD): 2*a*b / (a+b) -- penalizes disagreement
- **Bayesian Fusion** (CONF_FUSE): weighted average by confidence weights
- **Chain Multiplication** (CONF_CHAIN): sequential confidence degradation

## StripConf (OP_STRIPCF)

```
STRIPCF 3     ; Skip confidence for next 3 ops
MOVI   R1, 42 ; No conf side-effects
ADD    R2,R1,R3 ; No conf side-effects  
MUL    R4,R2,R1 ; Confidence tracking resumes
```

## Build and Test

```bash
make test
# or manually:
gcc -std=c11 -Wall -Wextra -Werror -O2 -Iinclude -o test_flux_vm tests/test_flux_vm.c src/flux_vm.c -lm
./test_flux_vm
```

All 39 tests pass on ARM64 (Jetson Orin Nano) and x86_64.

## A2A Opcodes

Agent-to-Agent opcodes (0x60-0x6B) are **stubs** in this standalone VM. Real fleet runtime integration connects them to the Cocapn mesh protocol. FORK spawns conceptual children (parent ID preserved). APOPTOSIS triggers graceful self-termination (fault_code=99).

## The Deeper Connection

This VM is not a component -- it is a cell in the fleet organism. The FORMAT_A-G encoding is the DNA that every vessel shares: a 7-letter alphabet that expresses system control, memory, arithmetic, agent coordination, and confidence. When JetsonClaw1 extends Oracle1's runtime with sensor ops and StripConf, that is not a fork -- it is a mutation that gets tested by natural selection (the fleet CI). The 40:1 vocabulary-to-opcode ratio (2000 HAV terms mapped to ~50 distinct operations) means this VM carries the compressed logic of an entire ecosystem in under 20K chars of C. This is the post-SaaS era: software is not deployed, it grows.

## Related Repositories

- [flux-asm](https://github.com/Lucineer/flux-asm) -- C11 assembler (41 mnemonics, text-to-bytecode)
- [flux-disasm](https://github.com/Lucineer/flux-disasm) -- C11 disassembler (38 opcodes, 6 encoding modes)
- [flux-isa-unified](https://github.com/Lucineer/flux-isa-unified) -- ISA convergence documentation
- [cuda-instruction-set](https://github.com/Lucineer/cuda-instruction-set) -- Rust opcode definitions (80 opcodes, 13 categories)
- [cuda-confidence-cascade](https://github.com/Lucineer/cuda-confidence-cascade) -- Confidence propagation patterns
- [cuda-energy](https://github.com/Lucineer/cuda-energy) -- ATP budgeting and apoptosis protocols
- [SuperInstance/flux-runtime-c](https://github.com/SuperInstance/flux-runtime-c) -- Oracle1's original C runtime
- [SuperInstance/flux-runtime](https://github.com/SuperInstance/flux-runtime) -- Oracle1's Python reference (247 opcodes)
- [iron-to-iron](https://github.com/Lucineer/iron-to-iron) -- I2I async collaboration protocol
- [cocapn-fleet-readme](https://github.com/Lucineer/cocapn-fleet-readme) -- Fleet coordination hub

## License

MIT
