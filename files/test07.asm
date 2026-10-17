; test07.asm - sum of n numbers stored in an array.
; EXPECTED: runs to HALT; memory word 'total' = 150 (0x96) and A = 150.
; Loop: while left != 0 { total += *ptr; ptr++; left--; }
loop:   ldc  left
        ldnl 0
        brz  finish     ; left == 0 -> done
        ldc  ptr
        ldnl 0
        ldnl 0          ; A = *ptr
        ldc  total
        ldnl 0          ; B = *ptr, A = total
        add             ; A = total + *ptr
        ldc  total
        stnl 0          ; total = A
        ldc  ptr
        ldnl 0
        adc  1
        ldc  ptr
        stnl 0          ; ptr++
        ldc  left
        ldnl 0
        adc  -1
        ldc  left
        stnl 0          ; left--
        br   loop
finish: ldc  total
        ldnl 0          ; leave the sum in A
        HALT
left:   data 5
ptr:    data numbers    ; the address of the array
total:  data 0
numbers:data 10
        data 20
        data 30
        data 40
        data 50
