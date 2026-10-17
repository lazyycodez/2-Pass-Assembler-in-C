; test05.asm - number formats, shifts, branches, label forms.
; EXPECTED: assembles; runs to HALT with A = 222, B = -13.
; Hex 0x10 (16) + octal 010 (8) + (-4) = 20.
; Arithmetic shift right: -64 >> 3 = -8.  B(-8) - 5 = -13  (< 0, so the
; brlz is taken and the 'ldc 111' is skipped).
start:  ldc 0x10        ; hexadecimal
        adc 010         ; octal
        adc -4          ; negative decimal    -> A = 20
        ldc -64
        ldc 3
        shr             ; A = -64 >> 3 = -8 (sign preserved)
        ldc 5
        sub             ; A = -8 - 5 = -13
        brlz negative   ; forward branch, taken
        ldc 111         ; skipped
        HALT
negative:ldc 222        ; label directly followed by a mnemonic
        HALT
