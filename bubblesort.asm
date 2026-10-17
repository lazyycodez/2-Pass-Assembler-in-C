; bubblesort.asm - bubble sort of an array in IITPSx assembly.
; Sorts 'count' words starting at 'arr' into ascending order, in place.
;
;   i = count - 1
;   while (i != 0) {
;       ptr = arr;  k = i
;       while (k != 0) {
;           if (ptr[1] < ptr[0]) swap(ptr[0], ptr[1])
;           ptr++;  k--
;       }
;       i--
;   }
;
; Each variable lives in memory. A and B form a 2-deep stack, so every
; value is fetched with "ldc var ; ldnl 0" just before it is needed.
; EXPECTED (emu -after): array = -15 -4 -4 0 3 8 8 17 29 100

        ldc  count
        ldnl 0
        adc  -1
        ldc  i
        stnl 0              ; i = count - 1

outer:  ldc  i
        ldnl 0
        brz  done           ; i == 0 -> sorted
        ldc  arr
        ldc  ptr
        stnl 0              ; ptr = arr
        ldc  i
        ldnl 0
        ldc  k
        stnl 0              ; k = i

inner:  ldc  k
        ldnl 0
        brz  nextpass       ; k == 0 -> end of this pass
        ldc  ptr
        ldnl 0
        ldnl 1              ; A = ptr[1]
        ldc  ptr
        ldnl 0
        ldnl 0              ; B = ptr[1], A = ptr[0]
        sub                 ; A = ptr[1] - ptr[0]
        brlz swap           ; ptr[1] < ptr[0] -> out of order

advance:ldc  ptr
        ldnl 0
        adc  1
        ldc  ptr
        stnl 0              ; ptr++
        ldc  k
        ldnl 0
        adc  -1
        ldc  k
        stnl 0              ; k--
        br   inner

swap:   ldc  ptr
        ldnl 0
        ldnl 0              ; A = ptr[0]
        ldc  tmp
        stnl 0              ; tmp = ptr[0]
        ldc  ptr
        ldnl 0
        ldnl 1              ; A = ptr[1]
        ldc  ptr
        ldnl 0              ; B = ptr[1], A = ptr
        stnl 0              ; ptr[0] = ptr[1]
        ldc  tmp
        ldnl 0              ; A = tmp
        ldc  ptr
        ldnl 0              ; B = tmp, A = ptr
        stnl 1              ; ptr[1] = tmp
        br   advance

nextpass:ldc i
        ldnl 0
        adc  -1
        ldc  i
        stnl 0              ; i--
        br   outer

done:   HALT

count:  data 10
i:      data 0
k:      data 0
ptr:    data 0
tmp:    data 0
arr:    data 29
        data -4
        data 17
        data 0
        data 8
        data -15
        data 8
        data 100
        data 3
        data -4
