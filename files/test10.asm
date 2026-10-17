; test10.asm - procedure call and return (call, return, adj, stl, ldl).
; square(9): the argument arrives in B and the return address in A.
; EXPECTED: HALT with A = 81 and SP back at its initial value 0x00010000.
        ldc  9
        call square
        HALT
square: adj  -2         ; make a 2-word frame
        stl  1          ; save return address (A) ; A = argument
        stl  0          ; save argument
        ldl  0          ; B = argument, A = argument
        mul             ; A = argument * argument
        ldl  1          ; B = result, A = return address
        adj  2          ; free the frame
        return          ; PC = return address, A = result
