# flux-runtime-c

**FLUX** — Fluid Language Universal eXecution. A C11 Micro-VM bytecode interpreter with 85 opcodes, A2A agent-to-agent protocol, SIMD support, and zero external dependencies.

Port of [SuperInstance/flux-runtime](https://github.com/SuperInstance/flux-runtime) from Python to C for maximum performance and portability. Compiles on ARM64 (Jetson), x86-64, and can target WASM or CUDA kernels.

## Quick Start

```bash
make test        # build + run 24 tests
make             # build flux binary
./build/flux     # run demo
```

## Architecture

### 85 Opcodes in 9 Groups

| Group | Opcodes | Examples |
|-------|---------|----------|
| Control | NOP, MOV, JMP, JZ, JNZ, CALL, RET, MOVI | Branching, function calls |
| Arithmetic | IADD, ISUB, IMUL, IDIV, IMOD, INEG, INC, DEC | Integer math |
| Bitwise | IAND, IOR, IXOR, INOT, ISHL, ISHR, ROTL, ROTR | Logic + rotates |
| Comparison | ICMP, IEQ, ILT, IGT, TEST, SETCC, JE, JNE, JL, JG | Flags + conditional jumps |
| Stack | PUSH, POP, DUP, SWAP, ROT, ENTER, LEAVE | Stack frame management |
| Memory | REGION_CREATE/DESTROY/TRANSFER, MEMCOPY, MEMSET | Capability-based regions |
| Type | CAST, BOX, UNBOX, CHECK_TYPE, CHECK_BOUNDS | Dynamic typing primitives |
| Float | FADD, FSUB, FMUL, FDIV, FNEG, FABS, FEQ, FLT | IEEE 754 single precision |
| SIMD | VLOAD, VSTORE, VADD, VSUB, VMUL, VDIV, VFMA | 4-wide int32/float SIMD |
| A2A | TELL, ASK, DELEGATE, BROADCAST, TRUST, CAP, BARRIER | Agent protocol |
| System | HALT, YIELD, RESOURCE_ACQUIRE/RELEASE, DEBUG_BREAK | VM lifecycle |

### Register File (64 registers)

- **R0-R15**: General purpose int32 (R11=SP, R14=FP, R15=LR)
- **F0-F15**: IEEE 754 float32
- **V0-V15**: 128-bit SIMD lanes (4×int32 or 4×float32)

### Memory Model

Linear regions with ownership semantics. Each region has a name, owner, and optional borrowers. The VM manages `stack` and `heap` regions automatically; agents create named regions for shared state.

### A2A Protocol

17 opcodes for inter-agent communication:
- **Messaging**: TELL, ASK, DELEGATE, BROADCAST, REDUCE
- **Trust**: TRUST_CHECK, TRUST_UPDATE, TRUST_QUERY, REVOKE_TRUST
- **Capabilities**: CAP_REQUIRE, CAP_REQUEST, CAP_GRANT, CAP_REVOKE
- **Coordination**: BARRIER, SYNC_CLOCK, FORMATION_UPDATE
- **Intent**: DECLARE_INTENT, ASSERT_GOAL, VERIFY_OUTCOME

## Instruction Encoding

Variable-length encoding (1-8 bytes):

```
Format A (1 byte): [opcode]
Format B (2 bytes): [opcode][rd]
Format C (3 bytes): [opcode][rd][rs1]
Format D (4 bytes): [opcode][rd][imm_lo][imm_hi]
Format E (4 bytes): [opcode][rd][rs1][rs2]
Format G (variable): [opcode][len_lo][len_hi][payload...]
```

## The Mitochondrion Connection

FLUX's bytecode interpreter IS cellular metabolism. Each opcode group maps to an instinct category:

| Instinct | Opcodes | Biological Analog |
|----------|---------|-------------------|
| Survive | HALT, RESOURCE_ACQUIRE, TRAP | Cellular homeostasis |
| Perceive | IO_READ, LOAD, CMP, TEST | Sensory transduction |
| Navigate | JMP, CALL, RET | Motor response |
| Communicate | A2A_SEND, BROADCAST | Cell signaling |
| Learn | BOX, UNBOX, CHECK_TYPE | Gene expression |
| Defend | REGION_CREATE, REGION_GUARD | Immune response |

*The VM is the mitochondrion. The bytecode is DNA. Opcodes are enzymes.*

## Building

```bash
make              # build flux binary
make test         # build + run all tests
make clean        # clean build artifacts
```

Zero dependencies — just a C11 compiler and libm. Tested on:
- ARM64 (NVIDIA Jetson Super Orin Nano)
- x86-64 (GCC, Clang)

## Tests

24 tests across VM and register subsystems:

```
VM Tests (20): nop, movi_add, mul_sub, div_mod, neg_inc_dec, bitwise,
  jz_branch, jnz_no_branch, push_pop, dup, call_ret, cmp_je, cast,
  float_ops, region_create, a2a_handler, box_unbox, halt_error,
  div_zero, cycle_budget

Register Tests (4): init, gp_read_write, sp_sync, fp_read_write
```

## License

MIT License. Copyright (c) 2024 SuperInstance (DiGennaro et al.). C port by Lucineer.

---

Part of the [Lucineer Fleet](https://github.com/Lucineer/the-fleet) — sovereign AI agent architecture.
