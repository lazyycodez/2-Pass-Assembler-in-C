; test13.asm - errant program: infinite loop.
; EXPECTED: assembles; emulator stops at its instruction limit and reports
; a probable infinite loop (try: emu -limit 1000 test13.o).
spin: br spin
