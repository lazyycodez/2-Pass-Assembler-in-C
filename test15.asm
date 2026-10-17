; test15.asm - errant program: load from outside memory.
; EXPECTED: assembles; emulator reports memory address 100000 out of range.
ldc 100000
ldnl 0
HALT
