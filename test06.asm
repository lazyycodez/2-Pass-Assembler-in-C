; test06.asm - additional assembler error tests (SET and operand checks).
; EXPECTED: all of these are reported and NO output is written:
;   operand too large for 24 bits, operand too small, SET without a label,
;   SET with bad value, SET with missing value, undefined labels,
;   label containing an underscore (not alphanumeric), over-long number.
ldc 99999999        ; does not fit in 24 bits
ldc -9000000        ; does not fit in 24 bits
SET 5               ; SET needs a label
bad: SET abc        ; SET value is not a number
nov: SET            ; missing operand
ldc ghost           ; no such label
br ghost2           ; no such label
a_b: add            ; underscore is not allowed in labels
ldc 99999999999999999999999 ; number out of range
