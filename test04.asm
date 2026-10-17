; test04.asm - spec sample 4: mul and div.
; EXPECTED: assembles; when executed HALT with A = 21.
ldc 6
ldc 7
mul ; A = 42
ldc 2
div ; A = 21
HALT
