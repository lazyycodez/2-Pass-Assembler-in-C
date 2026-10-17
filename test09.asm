; test09.asm - mean (integer average) of an array (uses div).
; Values 12 18 25 31 9 5 -> sum 100, count 6, mean = 100 / 6 = 16
; (signed division truncates toward zero).
; EXPECTED: HALT with A = 16; memory: total = 100 (0x64), mean = 16 (0x10).
loop:   ldc  left
        ldnl 0
        brz  average
        ldc  ptr
        ldnl 0
        ldnl 0
        ldc  total
        ldnl 0
        add
        ldc  total
        stnl 0          ; total += *ptr
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
average:ldc  total
        ldnl 0          ; A = total
        ldc  count
        ldnl 0          ; B = total, A = count
        div             ; A = total / count
        ldc  mean
        stnl 0          ; mean = A
        ldc  mean
        ldnl 0
        HALT
left:   data 6
count:  data 6
ptr:    data values
total:  data 0
mean:   data 0
values: data 12
        data 18
        data 25
        data 31
        data 9
        data 5
