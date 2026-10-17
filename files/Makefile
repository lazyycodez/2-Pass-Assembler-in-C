CC     = gcc
CFLAGS = -std=c89 -pedantic -W -Wall -Wpointer-arith -Wwrite-strings -Wstrict-prototypes

all: asm emu

asm: asm.c
	$(CC) $(CFLAGS) asm.c -o asm

emu: emu.c
	$(CC) $(CFLAGS) emu.c -o emu

# assemble + run every test, regenerating the .lst .o and .log files
test: all
	./run_tests.sh

clean:
	rm -f asm emu *.o *.lst *.log

.PHONY: all test clean
