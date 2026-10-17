; test02.asm - spec sample 2: error handling.
; EXPECTED: assembler reports every error below and writes NO output:
;   duplicate label, no such label, bad number, missing operand,
;   unexpected operand, extra text, bogus label name, bogus mnemonic (x2)
label:
label: ; duplicate label definition
br nonesuch ; no such label
ldc 08ge ; not a number
ldc ; missing operand
add 5 ; unexpected operand
ldc 5, 6; extra on end of line
0def: ; bogus label name
fibble; bogus mnemonic
0def ; bogus mnemonic
