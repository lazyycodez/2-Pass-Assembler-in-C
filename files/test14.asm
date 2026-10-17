; test14.asm - errant program: executing data as an instruction.
; EXPECTED: assembles; emulator reports "illegal opcode 127".
ldc 1
data 0x7F       ; low byte 0x7F is not an opcode
HALT
