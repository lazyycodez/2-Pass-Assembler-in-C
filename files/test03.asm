; test03.asm - spec sample 3: the SET pseudo-instruction.
; EXPECTED: assembles; ldc 75 (00004B00) then adc 66 (00004201).
; SET labels take their value from SET, not from the PC, and a forward
; reference to a SET label (val2) works because SET is handled in pass 1.
val: SET 75
ldc val
adc val2
val2: SET 66
HALT
