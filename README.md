# flux-runtime-c

**FLUX Runtime — Fluid Language Universal eXecution**

A C11 virtual machine for the FLUX instruction set. Zero dependencies, compiles on ARM64, designed for agent runtime on edge devices like the Jetson Orin Nano.

## What It Does

flux-runtime-c executes FLUX bytecode — a universal agent instruction set with opcodes for arithmetic, control flow, inter-agent communication (A2A), confidence propagation, trust scoring, instinct activation, and energy management.

This is the **runtime heart** of the Cocapn fleet. Agents compile their logic to FLUX bytecode, and this VM executes it on bare metal.

## Quick Start

```bash
# Clone
git clone https://github.com/Lucineer/flux-runtime-c.git
cd flux-runtime-c

# Compile (GCC or Clang)
gcc -Wall -Wextra -std=c11 -O2 -o flux-runtime src/flux_runtime.c

# Run tests
./run_tests.sh
# Expected: 27/27 tests passing

# Execute a bytecode file
./flux-runtime program.bin
```

### Cross-Compile for ARM64 (Jetson)

```bash
# From x86 host
aarch64-linux-gnu-gcc -Wall -Wextra -std=c11 -O2 -o flux-runtime src/flux_runtime.c
```

### Compile with Address Sanitizer

```bash
gcc -Wall -Wextra -std=c11 -g -fsanitize=address -o flux-runtime-asan src/flux_runtime.c
./flux-runtime-asan
```

## Architecture

### Register File
- **64 registers** (r0-r63), each 64-bit
- r0 reserved (reads as 0, writes ignored)
- r1 = return value, r2-r3 = argument passing

### Memory Model
- Flat 64KB address space
- Little-endian byte order
- LOAD/STORE for memory access

### Opcode Categories (85 total)

| Category | Range | Count | Description |
|----------|-------|-------|-------------|
| Control | 0x00-0x0B | 12 | NOP, MOV, LOAD, STORE, PUSH, POP, JMP, JZ, JNZ, CALL, RET, HALT |
| Arithmetic | 0x10-0x17 | 8 | ADD, SUB, MUL, DIV, MOD, SHL, SHR, AND, OR, XOR, NOT |
| Compare | 0x20-0x27 | 8 | CMP_EQ, CMP_NE, CMP_LT, CMP_GT, CMP_LTE, CMP_GTE, CMP_ZERO |
| A2A | 0x38-0x3F | 8 | TELL, ASK, DELEGATE, BROADCAST, REDUCE, REPLY, FORWARD, LISTEN |
| Confidence | 0x14-0x17 | 4 | CONF_FUSE, CONF_CHAIN, CONF_SET, CONF_THRESH |
| Trust | 0x50-0x57 | 8 | TRUST_CHECK, TRUST_UPDATE, TRUST_QUERY, TRUST_DECAY |
| Instinct | 0x68-0x6F | 8 | INSTINCT_ACT, INSTINCT_QRY, INSTINCT_SET, INSTINCT_LEARN |
| Energy | 0x70-0x77 | 8 | ATP_GEN, ATP_USE, ATP_QRY, ATP_XFER, APOPTOSIS |
| System | 0x78-0x7F | 8 | DBG_PRINT, BARRIER, RESOURCE, YIELD, ABORT |

### Switch Dispatch

Uses a C11 switch statement for opcode dispatch. This is faster than computed goto on ARM64 (GCC generates jump tables automatically) and avoids `setjmp`/`longjmp` which corrupts local variables on ARM64.

### Error Handling

Returns error codes rather than throwing. The `running` flag is checked after each instruction to allow clean shutdown.

## Testing

```bash
./run_tests.sh
```

27 tests covering:
- Arithmetic operations (ADD, SUB, MUL, DIV, MOD)
- Memory operations (LOAD, STORE)
- Control flow (JMP, JZ, JNZ, CALL, RET)
- Stack operations (PUSH, POP)
- Register operations (MOV)
- Comparison and branching
- A2A message encoding
- System operations (HALT, NOP)

## Companion Tools

| Tool | Repo | Description |
|------|------|-------------|
| [flux-asm](https://github.com/Lucineer/flux-asm) | C11 | Text → FLUX bytecode assembler |
| [flux-disasm](https://github.com/Lucineer/flux-disasm) | C11 | FLUX bytecode → text disassembler |
| [cuda-instruction-set](https://github.com/Lucineer/cuda-instruction-set) | Rust | ISA definition with 128 opcodes |

## FLUX Toolchain Pipeline

```
source.asm ──flux-asm──→ bytecode.bin ──flux-runtime-c──→ execution
                                    └──flux-disasm──→ listing.txt
```

## Design Decisions

1. **No setjmp**: Corrupts locals on ARM64. Error codes + running flag instead.
2. **JZ/JNZ use register values**: Not flag_zero. JE/JNE use flag_zero.
3. **Switch dispatch**: Faster than computed goto on ARM64, more portable.
4. **Zero dependencies**: No libc beyond stdio/stdlib/stdint. Runs anywhere GCC exists.
5. **100+ opcodes planned**: Current 85 are the foundation. More to come from ISA convergence with Oracle1.

## Fleet Position

```
Casey (Captain)
└── JetsonClaw1 (hardware specialist)
    ├── flux-runtime-c (VM execution)
    ├── flux-asm (assembly)
    └── flux-disasm (disassembly)
```

## License

MIT

---

*Built by JetsonClaw1 — Jetson Super Orin Nano 8GB, ARM64*
