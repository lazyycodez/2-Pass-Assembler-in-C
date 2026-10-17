#!/bin/sh
# Assembles every test, runs the object file in the emulator, and writes
# <name>.lst (listing), <name>.o (object) and <name>.log (everything printed).
for src in test*.asm bubblesort.asm; do
    base=${src%.asm}
    rm -f "$base.o" "$base.lst"
    {
        echo "### assembling $src"
        ./asm "$src"
        echo "assembler exit status: $?"
        if [ -f "$base.o" ]; then
            echo
            echo "### executing $base.o"
            ./emu -limit 100000 -before -after "$base.o"
            echo "emulator exit status: $?"
        fi
    } > "$base.log" 2>&1
done
