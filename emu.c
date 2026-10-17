/*****************************************************************************
TITLE: Emulator for the IITPSx processor
AUTHOR: <your name>          USER ID: <your id>
Declaration of Authorship:
I hereby declare that this source file is my own work.

USAGE:   emu [options] file.o
OPTIONS: -trace    print registers and the instruction before each step
         -before   dump the loaded memory image before running
         -after    dump the memory image after running
         -isa      print the instruction set and exit
         -limit N  stop after N instructions (default 10,000,000); this
                   catches infinite loops

MACHINE: registers A, B (an internal stack), PC and SP, 32 bits each.
         Memory is MEM_SIZE words; the program is loaded at address 0 and
         SP starts at MEM_SIZE (the stack grows downwards).
         The PC is incremented before an instruction executes, as in the
         specification.

EXIT:    0 = program executed HALT, 2 = runtime error, 1 = usage / load
         error.
*****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

/* Compile-time check: the emulator needs a 32-bit int. */
typedef char int_must_be_32_bits[(sizeof(int) == 4) ? 1 : -1];

#define MEM_SIZE      65536L        /* words of memory                    */
#define DEFAULT_LIMIT 10000000L     /* instructions before giving up      */

typedef int          Word;          /* signed 32-bit machine word         */
typedef unsigned int UWord;         /* same bits, unsigned (for wrapping) */

typedef struct {
    Word a, b, sp;
    long pc;                        /* long so bad jumps can be detected  */
} Cpu;

typedef struct {
    const char *name;
    int         has_operand;
} OpInfo;

/* Indexed by opcode. */
static const OpInfo op_table[] = {
    {"ldc", 1},  {"adc", 1},  {"ldl", 1},  {"stl", 1},  {"ldnl", 1},
    {"stnl", 1}, {"add", 0},  {"sub", 0},  {"shl", 0},  {"shr", 0},
    {"adj", 1},  {"a2sp", 0}, {"sp2a", 0}, {"call", 1}, {"return", 0},
    {"brz", 1},  {"brlz", 1}, {"br", 1},   {"HALT", 0}, {"mul", 0},
    {"div", 0}
};
#define NUM_OPS ((int)(sizeof op_table / sizeof op_table[0]))

enum { STEP_OK, STEP_HALT, STEP_ERROR };

static Word memory[MEM_SIZE];
static long image_size = 0;         /* words loaded from the object file  */
static char error_text[160];        /* description of a runtime error     */

/* ------------------------------------------------------------------ */
/* Loading and dumping                                                 */
/* ------------------------------------------------------------------ */

/* Reads the object file (4 bytes per word, least significant first).
   Returns 1 on success, 0 on failure after printing a message. */
static int load_object(const char *name)
{
    FILE *f = fopen(name, "rb");
    int   c0, c1, c2, c3;
    UWord w;

    if (f == NULL) {
        fprintf(stderr, "cannot open '%s'\n", name);
        return 0;
    }
    while ((c0 = getc(f)) != EOF) {
        c1 = getc(f);
        c2 = getc(f);
        c3 = getc(f);
        if (c1 == EOF || c2 == EOF || c3 == EOF) {
            fprintf(stderr, "'%s' is not a valid object file "
                            "(size is not a multiple of 4)\n", name);
            fclose(f);
            return 0;
        }
        if (image_size >= MEM_SIZE) {
            fprintf(stderr, "'%s' is larger than memory (%ld words)\n",
                    name, MEM_SIZE);
            fclose(f);
            return 0;
        }
        w = (UWord)c0 | ((UWord)c1 << 8) | ((UWord)c2 << 16)
            | ((UWord)c3 << 24);
        memory[image_size++] = (Word)w;
    }
    fclose(f);
    return 1;
}

/* Prints memory[lo .. hi-1], four words per line. */
static void dump_memory(long lo, long hi)
{
    long i;
    for (i = lo; i < hi; i++) {
        if ((i - lo) % 4 == 0)
            printf("%s%08lX", (i == lo) ? "" : "\n", (unsigned long)i);
        printf(" %08X", (UWord)memory[i]);
    }
    printf("\n");
}

static void print_isa(void)
{
    int i;
    printf("Opcode Mnemonic Operand\n");
    for (i = 0; i < NUM_OPS; i++)
        printf("%6d %-8s %s\n", i, op_table[i].name,
               op_table[i].has_operand ? "value/offset" : "-");
}

/* ------------------------------------------------------------------ */
/* Execution                                                           */
/* ------------------------------------------------------------------ */

/* Operand = upper 24 bits, sign-extended to 32 bits. */
static Word sign_extend_24(UWord field)
{
    Word v = (Word)(field & 0xFFFFFFUL);
    if (v >= 0x800000)
        v -= 0x1000000;
    return v;
}

/* Returns 1 (and sets error_text) if addr is outside memory. */
static int bad_address(long addr, long fetch_pc)
{
    if (addr >= 0 && addr < MEM_SIZE)
        return 0;
    sprintf(error_text, "memory address %ld out of range at PC=%08lX",
            addr, (unsigned long)fetch_pc);
    return 1;
}

/* Executes one instruction. Returns STEP_OK, STEP_HALT or STEP_ERROR. */
static int step(Cpu *cpu)
{
    long  fetch_pc = cpu->pc, addr;
    UWord instr;
    int   opcode;
    Word  operand, count;

    if (fetch_pc < 0 || fetch_pc >= MEM_SIZE) {
        sprintf(error_text, "PC out of range (%ld)", fetch_pc);
        return STEP_ERROR;
    }
    instr = (UWord)memory[fetch_pc];
    cpu->pc++;                               /* PC moves on first */
    opcode  = (int)(instr & 0xFFU);
    operand = sign_extend_24(instr >> 8);

    switch (opcode) {
    case 0:  /* ldc */
        cpu->b = cpu->a;
        cpu->a = operand;
        break;
    case 1:  /* adc */
        cpu->a = (Word)((UWord)cpu->a + (UWord)operand);
        break;
    case 2:  /* ldl */
        addr = (long)cpu->sp + operand;
        if (bad_address(addr, fetch_pc)) return STEP_ERROR;
        cpu->b = cpu->a;
        cpu->a = memory[addr];
        break;
    case 3:  /* stl */
        addr = (long)cpu->sp + operand;
        if (bad_address(addr, fetch_pc)) return STEP_ERROR;
        memory[addr] = cpu->a;
        cpu->a = cpu->b;
        break;
    case 4:  /* ldnl */
        addr = (long)cpu->a + operand;
        if (bad_address(addr, fetch_pc)) return STEP_ERROR;
        cpu->a = memory[addr];
        break;
    case 5:  /* stnl */
        addr = (long)cpu->a + operand;
        if (bad_address(addr, fetch_pc)) return STEP_ERROR;
        memory[addr] = cpu->b;
        break;
    case 6:  /* add */
        cpu->a = (Word)((UWord)cpu->b + (UWord)cpu->a);
        break;
    case 7:  /* sub */
        cpu->a = (Word)((UWord)cpu->b - (UWord)cpu->a);
        break;
    case 8:  /* shl */
        count = cpu->a;
        if (count < 0 || count > 31) {
            sprintf(error_text, "invalid shift count %d at PC=%08lX",
                    count, (unsigned long)fetch_pc);
            return STEP_ERROR;
        }
        cpu->a = (Word)((UWord)cpu->b << count);
        break;
    case 9:  /* shr (arithmetic: the sign bit is copied in) */
        count = cpu->a;
        if (count < 0 || count > 31) {
            sprintf(error_text, "invalid shift count %d at PC=%08lX",
                    count, (unsigned long)fetch_pc);
            return STEP_ERROR;
        }
        if (cpu->b >= 0)
            cpu->a = cpu->b >> count;
        else
            cpu->a = ~((~cpu->b) >> count);
        break;
    case 10: /* adj */
        cpu->sp = (Word)((UWord)cpu->sp + (UWord)operand);
        break;
    case 11: /* a2sp */
        cpu->sp = cpu->a;
        cpu->a = cpu->b;
        break;
    case 12: /* sp2a */
        cpu->b = cpu->a;
        cpu->a = cpu->sp;
        break;
    case 13: /* call */
        cpu->b = cpu->a;
        cpu->a = (Word)cpu->pc;
        cpu->pc += operand;
        break;
    case 14: /* return */
        cpu->pc = cpu->a;
        cpu->a = cpu->b;
        break;
    case 15: /* brz */
        if (cpu->a == 0)
            cpu->pc += operand;
        break;
    case 16: /* brlz */
        if (cpu->a < 0)
            cpu->pc += operand;
        break;
    case 17: /* br */
        cpu->pc += operand;
        break;
    case 18: /* HALT */
        return STEP_HALT;
    case 19: /* mul: low 32 bits of the product, computed unsigned so
                that overflow wraps instead of being undefined */
        cpu->a = (Word)((UWord)cpu->b * (UWord)cpu->a);
        break;
    case 20: /* div: signed, truncates toward zero */
        if (cpu->a == 0) {
            sprintf(error_text, "division by zero at PC=%08lX",
                    (unsigned long)fetch_pc);
            return STEP_ERROR;
        }
        if (cpu->a == -1 && cpu->b == INT_MIN) {
            sprintf(error_text, "division overflow at PC=%08lX",
                    (unsigned long)fetch_pc);
            return STEP_ERROR;
        }
        cpu->a = cpu->b / cpu->a;
        break;
    default:
        sprintf(error_text, "illegal opcode %d at PC=%08lX", opcode,
                (unsigned long)fetch_pc);
        return STEP_ERROR;
    }
    return STEP_OK;
}

/* Prints registers and the instruction about to be executed. */
static void print_trace(const Cpu *cpu)
{
    UWord instr;
    int   opcode;

    printf("PC=%08lX SP=%08X A=%08X B=%08X  ", (unsigned long)cpu->pc,
           (UWord)cpu->sp, (UWord)cpu->a, (UWord)cpu->b);
    if (cpu->pc >= 0 && cpu->pc < MEM_SIZE) {
        instr  = (UWord)memory[cpu->pc];
        opcode = (int)(instr & 0xFFU);
        if (opcode < NUM_OPS) {
            printf("%s", op_table[opcode].name);
            if (op_table[opcode].has_operand)
                printf(" %d", sign_extend_24(instr >> 8));
        } else {
            printf("??? (opcode %d)", opcode);
        }
    }
    printf("\n");
}

/* Runs until HALT, an error, or the instruction limit. */
static int run(Cpu *cpu, long limit, int trace, long *executed)
{
    int status;

    *executed = 0;
    for (;;) {
        if (*executed >= limit) {
            sprintf(error_text, "instruction limit (%ld) reached - "
                                "probable infinite loop", limit);
            return STEP_ERROR;
        }
        if (trace)
            print_trace(cpu);
        status = step(cpu);
        (*executed)++;
        if (status != STEP_OK)
            return status;
    }
}

int main(int argc, char *argv[])
{
    Cpu   cpu;
    long  limit = DEFAULT_LIMIT, executed = 0;
    int   trace = 0, before = 0, after = 0, status, i;
    char *end;
    const char *file = NULL;

    for (i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-trace") == 0) {
            trace = 1;
        } else if (strcmp(argv[i], "-before") == 0) {
            before = 1;
        } else if (strcmp(argv[i], "-after") == 0) {
            after = 1;
        } else if (strcmp(argv[i], "-isa") == 0) {
            print_isa();
            return 0;
        } else if (strcmp(argv[i], "-limit") == 0 && i + 1 < argc) {
            limit = strtol(argv[++i], &end, 10);
            if (*end != '\0' || limit <= 0) {
                fprintf(stderr, "bad instruction limit '%s'\n", argv[i]);
                return 1;
            }
        } else if (argv[i][0] == '-') {
            fprintf(stderr, "unknown option '%s'\n", argv[i]);
            return 1;
        } else {
            file = argv[i];
        }
    }
    if (file == NULL) {
        fprintf(stderr, "usage: %s [-trace] [-before] [-after] [-isa] "
                        "[-limit N] file.o\n", argv[0]);
        return 1;
    }
    if (!load_object(file))
        return 1;
    printf("loaded %ld words from %s\n", image_size, file);
    if (before) {
        printf("memory dump before execution:\n");
        dump_memory(0, image_size);
    }

    cpu.a = 0;
    cpu.b = 0;
    cpu.pc = 0;
    cpu.sp = (Word)MEM_SIZE;

    status = run(&cpu, limit, trace, &executed);
    if (status == STEP_HALT)
        printf("HALT reached after %ld instructions\n", executed);
    else
        printf("RUNTIME ERROR: %s (after %ld instructions)\n",
               error_text, executed);
    printf("A=%d (0x%08X)  B=%d (0x%08X)  PC=%08lX  SP=%08X\n",
           cpu.a, (UWord)cpu.a, cpu.b, (UWord)cpu.b,
           (unsigned long)cpu.pc, (UWord)cpu.sp);
    if (after) {
        printf("memory dump after execution:\n");
        dump_memory(0, image_size);
    }
    return (status == STEP_HALT) ? 0 : 2;
}
