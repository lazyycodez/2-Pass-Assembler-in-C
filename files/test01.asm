; test01.asm - spec sample 1: valid but nonsense program.
; EXPECTED: assembles with no errors; ONE warning (unused label 'label').
; Checks: unused label, label alone on a line, decimal/signed operands,
; backward branch (offset -1), zero-offset branch, label as operand,
; forward reference, data pseudo-op.
label: ; an unused label
ldc 0
ldc -5
ldc +5
loop: br loop ; an infinite loop
br next ;offset should be zero
next:
ldc loop ; load code address
ldc var1 ; forward ref
var1: data 0 ; a variable
