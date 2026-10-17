; test12.asm - errant program: jump outside memory.
; EXPECTED: assembles; emulator reports "PC out of range (70000)".
ldc 70000
return          ; PC = A = 70000, beyond the 65536-word memory
HALT
