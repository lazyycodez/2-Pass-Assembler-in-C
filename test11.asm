; test11.asm - runtime error: division by zero.
; EXPECTED: assembles; emulator reports "division by zero" and exits 2.
ldc 10
ldc 0
div
HALT
