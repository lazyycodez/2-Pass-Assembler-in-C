; test08.asm - polynomial evaluation with Horner's rule (uses mul).
; p(x) = 3x^2 + 2x + 1 at x = 5  ->  3*25 + 10 + 1 = 86.
; EXPECTED: HALT with A = 86; memory word 'acc' = 86 (0x56).
; acc = coef[0]; for each remaining coefficient: acc = acc * x + coef
        ldc  coef
        ldnl 0
        ldc  acc
        stnl 0          ; acc = coef[0]
loop:   ldc  left
        ldnl 0
        brz  finish
        ldc  acc
        ldnl 0          ; A = acc
        ldc  x
        ldnl 0          ; B = acc, A = x
        mul             ; A = acc * x
        ldc  ptr
        ldnl 0
        ldnl 0          ; B = acc * x, A = next coefficient
        add             ; A = acc*x + coefficient
        ldc  acc
        stnl 0          ; acc = A
        ldc  ptr
        ldnl 0
        adc  1
        ldc  ptr
        stnl 0
        ldc  left
        ldnl 0
        adc  -1
        ldc  left
        stnl 0
        br   loop
finish: ldc  acc
        ldnl 0
        HALT
x:      data 5
left:   data 2          ; degree = number of coefficients after the first
ptr:    data coef2
acc:    data 0
coef:   data 3
coef2:  data 2
        data 1
