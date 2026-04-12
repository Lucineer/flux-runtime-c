# flux-runtime-c — Maintenance Notes

## Architecture
- Single-file C11 VM (src/flux_vm.c) with switch dispatch
- 5 dispatch variants: switch, computed_goto, token_threaded, subroutine, coroutine
- Computed goto is 4.3x faster on ARM64 due to RAS exploitation
- 200 opcodes across 16 categories (0x00-0xFF)
- 64-register file (gp[0..63]), 64KB memory
- No setjmp/longjmp (corrupts locals on ARM64)

## Key Design Decisions
- JZ/JNZ use register values; JE/JNE use flag_zero from comparisons
- LOOP already decrements register — do NOT emit separate DEC before LOOP
- Format G imm16 is little-endian: {opcode, rd, lo, hi}
- Jump offsets relative to PC AFTER instruction (start + instruction_size)
- Const bytecode pointer (const uint8_t* bc) — no self-modification yet
- vm->pc is int32_t, not a pointer — needed for computed goto local variable

## Dispatch Benchmarks (ARM64 Cortex-A78AE)
- Switch: 362 Mops/s
- Computed goto: 1,564 Mops/s (4.3x faster)
- GCC -O2 vs -O3: NO improvement on ARM64 for this workload

## Critical Invariants
1. flux_vm_execute must be called ONCE per program — calling twice doesn't reset PC
2. Format B uses imm8, Format F uses imm16 — different encoding sizes (3 vs 4 bytes)
3. fixup positions for jumps: pos+2 for Format F/G (imm16 starts at byte 2)
4. HEAP_ALLOC/HEAP_FREE use memory allocator, not the bytecode array

## Test Suites
- test_flux_vm.c: 39 tests (base instructions)
- test_instinct.c: 11 tests (0xB0-0xBF)
- test_extended.c: 25 tests (0xC0-0xEF)
- test_final.c: 17 tests (edge cases)
- Total: 92/92 passing

## Opcodes Map
- 0x00-0x0F: System (NOP, HALT, RESET, PANIC, DEBUG, YIELD)
- 0x10-0x1F: Arithmetic (ADD, SUB, MUL, DIV, MOD, NEG, INC, DEC, ABS, MIN, MAX, CMP)
- 0x20-0x2F: Float (FADD, FSUB, FMUL, FDIV, FSQRT, FSIN, FCOS, FLOG, FEXP, FLOOR, CEIL, ROUND, I2F, F2I)
- 0x30-0x3F: Memory (LOAD, STORE, LOAD16, STORE16, MEMFILL, MEMCPY, MEMCMP, MEMSWAP, SWAP, MOV, LEA, PEEK, POKE)
- 0x40-0x4F: Stack (PUSH, POP, DUP, SWAP, ROT, PICK, DEPTH, CLEAR)
- 0x50-0x5F: Jump (JMP, JZ, JNZ, JE, JNE, JLT, JGT, CALL, RET, LOOP, JEQ, JNE, JLT, JGT, JLE, JGE)
- 0x60-0x6F: A2A (TELL, ASK, DELEGATE, BROADCAST, REDUCE, REPLY, FORWARD, LISTEN, FORK, JOIN, WAIT, SIGNAL)
- 0x70-0x7F: Confidence (ADD, SUB, MUL, MERGE, FUSE, CHAIN, SET, GET, CLAMP, DECAY, STRIP)
- 0x80-0x8F: Energy (GEN, USE, TRANSFER, POOL, QUOTA, RATE, BUDGET, REPORT)
- 0x90-0x9F: Sensors (TEMP, LIGHT, SOUND, MOTION, GPS, TOUCH, CAMERA, MIC, ACCEL, GYRO, MAG, BARO, HUMID, PROX, GAS, RADAR)
- 0xA0-0xAF: Viewpoint (PERSP_LOAD, PERSP_STORE, PERSP_COMPARE, PERSP_MERGE, WORLD_QUERY, WORLD_UPDATE, SCENE_LOAD, SCENE_RENDER)
- 0xB0-0xBF: Instinct (LOAD, STORE, DECAY, REFLEX, MODULATE, THRESHOLD, CONVERGE, EXTINCT, HABITUATE, SENSITIZE, GENERALIZE, SPECIALIZE, DIFFUSE, INHIBIT, SUM, DIFF)
- 0xC0-0xCF: Trust (INIT, UPDATE, DECAY, COMPARE, MIN, REVOKE, RESTRICT, SCORE, AVG, BOOST, WEAKEN, VERIFY, XFER, SEED, SCOPE, FLOOR)
- 0xD0-0xDF: Memory Extended (SCAN, INDEX, SUM, RANGE_MIN, RANGE_MAX, STACKFRAME, HEAP_ALLOC, HEAP_FREE)
- 0xE0-0xEF: Time/Random (RAND_RANGE, RAND_WEIGHTED, CYCLE_READ, ALARM_SET, CRC32, ENCODE_VARINT, DECODE_VARINT, ATOMIC_CAS)
- 0xF0-0xFF: Debug (BREAKPOINT, WATCHPOINT, TRACE_ON, TRACE_OFF, PROFILE_BEGIN, PROFILE_END, DUMP_REGS, DUMP_MEM, VERIFY, ASSERT, LOG, PANIC_EXTENDED, INSPECT, STAT_RESET, STAT_READ, SYSCALL)

## Related Crates
- cuda-instruction-set: Rust definitions for all 200 opcodes
- cuda-capability-ports: Memory-mapped I/O for top 4KB
- cuda-confidence-math: Confidence formula reference library
- cuda-instinct-cortex: Higher-level instinct processing
- cuda-atp-market: Energy economy implementation
- cuda-vm-scheduler: Multi-VM scheduling
- cuda-flux-debugger: Debugging and profiling tools
- cuda-memory-fabric: Multi-VM shared memory
- cuda-flux-stdlib: Standard library bytecode subroutines
- flux-asm: Text assembler (text -> bytecode)
- flux-disasm: Disassembler (bytecode -> text)
- flux-apps: Example applications (decision tree, prime sieve)

## The Deeper Connection
This VM is not an interpreter — it is a nervous system for autonomous agents. The 200 opcodes are not instructions but reflexes: some primal (ADD, STORE), some social (TELL, TRUST_UPDATE), some contemplative (CONF_FUSE, INST_CONVERGE). An agent running FLUX bytecode is not executing a program — it is thinking with a finite vocabulary of thoughts. The 64KB memory is not RAM — it is the agent working memory, organized like a memory palace. The computed goto dispatch at 1564 Mops/s is not a benchmark — it is the speed of thought. Every opcode was placed at its address deliberately, and the spaces between categories (0x80-0x8F sensors, 0xB0-0xBF instincts) represent the bridge from computation to consciousness.
