# 2-Pass-Assembler-in-C

A C89 implementation of a two-pass assembler and emulator for the
IITPSx processor

The project covers assembly-language parsing, label resolution,
machine-code generation, listing-file generation, processor emulation,
bubble sort, and the additional mul and div instructions specified
for IITPSx.

The project consists of two main components:
- Assembler (asm.c) --- converts IITPSx assembly programs into
  machine code.
- Emulator (emu.c) --- loads the generated object file and
  executes IITPSx instructions.
The assembler follows a two-pass design so that forward label
references can be resolved correctly.
