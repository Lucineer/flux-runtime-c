# flux-runtime-c

**The mitochondrion made manifest - a C11 bytecode VM.**

> Bytecode is DNA. Opcodes are enzymes. Fetch-decode-execute is cellular metabolism.

## What It Is

A portable C11 bytecode interpreter that runs agent programs. Zero dependencies (just libc). Compiles on ARM64, x86-64, WASM, and embedded targets.

### Specs

- 85 opcodes across 13 categories
- 64-register file
- Switch-based dispatch
- Harvard architecture (separate code/data memory)
- 27/27 tests passing on ARM64 Jetson

### Opcodes Include

- Arithmetic: ADD, SUB, MUL, DIV, MOD, AND, OR, XOR, SHL, SHR
- Memory: LOAD, STORE, BOX, UNBOX
- Control: JMP, JZ, JNZ, JE, JNE, CALL, RET
- I/O: TELL, ASK, LISTEN
- Instinct: SURVIVE, PERCEIVE, NAVIGATE, COMMUNICATE, LEARN, DEFEND
- Energy: ATP_GEN, ATP_CHECK, APOPTOSIS

## The Biological Connection

Each cuda-genepool instinct maps to a VM instruction group:
- Survive -> HALT, TRAP
- Perceive -> IO_READ, LOAD, CMP
- Navigate -> JMP, CALL, RET
- Communicate -> TELL, BROADCAST
- Learn -> BOX, MOV
- Defend -> REGION_GUARD, VERIFY

## Available In

- **C** (`flux-runtime-c`) - Primary, zero deps
- **Rust** (`cuda-instruction-set`) - Same opcodes as Rust types
- **Python** (`flux-runtime`) - Original Python reference

## Ecosystem Integration

- `cuda-genepool` - Mitochondrial engine generates FLUX bytecode
- `cuda-instruction-set` - Rust type definitions for all opcodes
- `cuda-assembler` - Text-to-bytecode assembler
- `cuda-biology` - BiologicalAgent uses instinct opcodes

## Building

```bash
gcc -Wall -Wextra -std=c11 -O2 -o test_vm tests/test_vm.c src/vm.c -lm
./test_vm
```

## See Also

- [cuda-instruction-set](https://github.com/Lucineer/cuda-instruction-set) - Opcode definitions
- [cuda-assembler](https://github.com/Lucineer/cuda-assembler) - Text assembler
- [cuda-genepool](https://github.com/Lucineer/cuda-genepool) - Biological engine
- [cuda-biology](https://github.com/Lucineer/cuda-biology) - Biological agent

## License

MIT OR Apache-2.0