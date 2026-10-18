//TITLE: Emulator for the IITPSx processor
//AUTHOR: <Yash Trivedi>          ROLL NO: <2501CS53>
//Declaration of Authorship:
// I hereby declare that this source file is my own work.


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>
#include <errno.h>

#define MAX_LINE 256 /* longest source line we accept               */
#define MAX_NAME 64  /* longest label / mnemonic / operand + 1      */
#define MAX_labelS 1024
#define MAX_CODE 4096 /* max words in an assembled program           */
#define MAX_TEXT 160  /* longest listing text for one statement      */
#define MAX_LIST (MAX_CODE + 2 * MAX_labelS)
#define MAX_DIAGS 256 /* max stored errors + warnings                */
#define MAX_DIAG_TEXT 200

#define OP_DATA (-1) /* pseudo-op: reserve a word                   */
#define OP_set (-2)  /* pseudo-op: give a label a direct_val             */

typedef enum
{
    none,
    direct_val,
    offset
} opkind;

typedef struct
{
    const char *mnemonic;
    int opcode;
    opkind kind;
} instruction;

static const instruction instr_table[] = {
    {"ldc", 0, direct_val}, {"adc", 1, direct_val}, {"ldl", 2, direct_val}, {"stl", 3, direct_val}, {"ldnl", 4, direct_val}, {"stnl", 5, direct_val}, {"add", 6, none}, {"sub", 7, none}, {"shl", 8, none}, {"shr", 9, none}, {"adj", 10, direct_val}, {"a2sp", 11, none}, {"sp2a", 12, none}, {"call", 13, offset}, {"return", 14, none}, {"brz", 15, offset}, {"brlz", 16, offset}, {"br", 17, offset}, {"HALT", 18, none}, {"mul", 19, none}, {"div", 20, none}, {"data", OP_DATA, direct_val}, {"set", OP_set, direct_val}};

#define num (sizeof instr_table / sizeof instr_table[0])

typedef struct
{
    char name[MAX_NAME];
    long direct_val; 
    int is_set;      
    int used;        
    int line;        
} label;

static label label_table[MAX_labelS];
static int label_count = 0;

static unsigned long code[MAX_CODE]; /* assembled words (32 bits used) */
static long code_size = 0;           /* number of words produced       */

typedef struct
{
    long address; /* -1: no address column                */
    int has_word;
    unsigned long word;
    char text[MAX_TEXT];
} ListEntry;

static ListEntry list_table[MAX_LIST];
static int list_count = 0;

/* Index of the label called name, or -1. */
static int find_label(const char *name)
{
    int i;
    for (i = 0; i < label_count; i++)
        if (strcmp(label_table[i].name, name) == 0)
            return i;
    return -1;
}

/* Index of a (non-set) label located at address, or -1. Used by the
   listing to print "br loop" instead of a raw offset. */
static int find_label_at(long address)
{
    int i;
    for (i = 0; i < label_count; i++)
        if (!label_table[i].is_set && label_table[i].direct_val == address)
            return i;
    return -1;
}

/* Table entry for a mnemonic, or NULL if unknown. */
static const instruction *find_instr(const char *mnemonic)
{
    size_t i;
    for (i = 0; i < num; i++)
        if (strcmp(instr_table[i].mnemonic, mnemonic) == 0)
            return &instr_table[i];
    return NULL;
}

static void add_list(long address, int has_word, unsigned long word,
                     const char *text)
{
    if (list_count >= MAX_LIST)
        return;
    list_table[list_count].address = address;
    list_table[list_count].has_word = has_word;
    list_table[list_count].word = word;
    strcpy(list_table[list_count].text, text);
    list_count++;
}

/* ------------------------------------------------------------------ */
/* 3. Diagnostics (errors and warnings), printed sorted by line        */
/* ------------------------------------------------------------------ */

typedef struct
{
    int line;
    int is_warning;
    char text[MAX_DIAG_TEXT];
} Diag;

static Diag diags[MAX_DIAGS];
static int diag_count = 0, error_count = 0, warning_count = 0;

/* Callers only pass format strings whose arguments are names of at most
   MAX_NAME characters, so the fixed buffer can not overflow. */
static void add_diag(int line_no, int is_warning, const char *fmt,
                     va_list args)
{
    if (is_warning)
        warning_count++;
    else
        error_count++;
    if (diag_count < MAX_DIAGS)
    {
        diags[diag_count].line = line_no;
        diags[diag_count].is_warning = is_warning;
        vsprintf(diags[diag_count].text, fmt, args);
        diag_count++;
    }
}

static void report_error(int line_no, const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    add_diag(line_no, 0, fmt, args);
    va_end(args);
}

static void report_warning(int line_no, const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    add_diag(line_no, 1, fmt, args);
    va_end(args);
}

/* Stable insertion sort by line number, then print on stderr. */
static void print_diags(void)
{
    int i, j;
    Diag tmp;

    for (i = 1; i < diag_count; i++)
    {
        tmp = diags[i];
        j = i - 1;
        while (j >= 0 && diags[j].line > tmp.line)
        {
            diags[j + 1] = diags[j];
            j--;
        }
        diags[j + 1] = tmp;
    }
    for (i = 0; i < diag_count; i++)
        fprintf(stderr, "%s line %d: %s\n",
                diags[i].is_warning ? "WARNING" : "ERROR",
                diags[i].line, diags[i].text);
    if (error_count + warning_count > diag_count)
        fprintf(stderr, "(further messages suppressed)\n");
}

/* ------------------------------------------------------------------ */
/* 4. Line parsing                                                     */
/* ------------------------------------------------------------------ */

typedef struct
{
    char label[MAX_NAME];
    char mnemonic[MAX_NAME];
    char operand[MAX_NAME];
    int has_label, has_mnemonic, has_operand, has_extra;
} ParsedLine;

/* Removes leading/trailing whitespace in place; returns start of result. */
static char *trim(char *s)
{
    char *end;
    while (isspace((unsigned char)*s))
        s++;
    end = s + strlen(s);
    while (end > s && isspace((unsigned char)end[-1]))
        end--;
    *end = '\0';
    return s;
}

/* A valid label is alphanumeric and starts with a letter. */
static int is_valid_label(const char *s)
{
    if (!isalpha((unsigned char)s[0]))
        return 0;
    for (s++; *s != '\0'; s++)
        if (!isalnum((unsigned char)*s))
            return 0;
    return 1;
}

/* Splits one source line into label / mnemonic / operand. Does NOT decide
   whether they are valid - that is done by the pass routine. */
static void parse_line(char *line, ParsedLine *p)
{
    char *comment, *colon, *rest;
    char extra[MAX_NAME];
    int n;

    memset(p, 0, sizeof *p);

    comment = strchr(line, ';'); /* drop the comment            */
    if (comment != NULL)
        *comment = '\0';

    rest = line;
    colon = strchr(line, ':'); /* label definition?           */
    if (colon != NULL)
    {
        *colon = '\0';
        strncpy(p->label, trim(line), MAX_NAME - 1);
        p->has_label = 1;
        rest = colon + 1;
    }

    n = sscanf(rest, "%63s %63s %63s", p->mnemonic, p->operand, extra);
    p->has_mnemonic = (n >= 1);
    p->has_operand = (n >= 2);
    p->has_extra = (n >= 3);
}

/* Error message if the number of operands is wrong, else NULL. */
static const char *check_operands(const instruction *def, const ParsedLine *p)
{
    if (p->has_extra)
        return "extra text at end of line";
    if (def->kind == none && p->has_operand)
        return "unexpected operand";
    if (def->kind != none && !p->has_operand)
        return "missing operand";
    return NULL;
}

/* ------------------------------------------------------------------ */
/* 5. Numbers, operands and encoding                                   */
/* ------------------------------------------------------------------ */

/* Converts decimal / 0x hex / 0 octal text (optional sign).
   Returns 0 ok, 1 not a number, 2 out of range. */
static int parse_number(const char *text, long *direct_val)
{
    char *end;
    errno = 0;
    *direct_val = strtol(text, &end, 0);
    if (end == text || *end != '\0')
        return 1;
    if (errno == ERANGE)
        return 2;
    return 0;
}

/* Turns operand text into a number. Operands starting with a letter are
   labels. Returns 0 ok, 1 no such label, 2 not a number, 3 out of range.
   *was_label tells the caller what the operand was. */
static int resolve_operand(const char *text, long *direct_val, int *was_label)
{
    int idx, rc;

    if (isalpha((unsigned char)text[0]))
    {
        *was_label = 1;
        idx = find_label(text);
        if (idx < 0)
            return 1;
        label_table[idx].used = 1;
        *direct_val = label_table[idx].direct_val;
        return 0;
    }
    *was_label = 0;
    rc = parse_number(text, direct_val);
    return (rc == 0) ? 0 : rc + 1;
}

/* Machine word: operand in the top 24 bits, opcode in the bottom 8.
   Masking to 24 bits stores a negative operand in two's complement.
   The data pseudo-op emits the raw 32-bit direct_val. */
static unsigned long encode(const instruction *def, long direct_val)
{
    if (def->opcode == OP_DATA)
        return (unsigned long)direct_val & 0xFFFFFFFFUL;
    return (((unsigned long)direct_val & 0xFFFFFFUL) << 8) | (unsigned long)def->opcode;
}

/* Builds the mnemonic/operand text shown in the listing. A branch to a
   label shows the label; a numeric branch offset is converted into the
   target address and shown as a label if one lives there. */
static void make_text(char *dst, const instruction *def, const ParsedLine *p,
                      long address, long direct_val, int was_label)
{
    long target;
    int idx;

    if (def->kind == none)
    {
        strcpy(dst, def->mnemonic);
    }
    else if (def->kind == offset && !was_label)
    {
        target = address + 1 + direct_val;
        idx = find_label_at(target);
        if (idx >= 0)
            sprintf(dst, "%s %s", def->mnemonic, label_table[idx].name);
        else
            sprintf(dst, "%s %ld", def->mnemonic, direct_val);
    }
    else
    {
        sprintf(dst, "%s %s", def->mnemonic, p->operand);
    }
}

/* ------------------------------------------------------------------ */
/* 6. The single routine used for both passes                          */
/* ------------------------------------------------------------------ */

/* Pass 1: defines a label found on the current line. A label on a set
   line gets the set direct_val, any other label gets the current address. */
static void define_label(const ParsedLine *p, const instruction *def,
                         int line_no, long address)
{
    long direct_val = address;
    int is_set = (def != NULL && def->opcode == OP_set);
    int rc;

    if (is_set)
    {
        direct_val = 0;
        if (p->has_operand)
        {
            rc = parse_number(p->operand, &direct_val);
            if (rc != 0)
            {
                report_error(line_no, "set direct_val '%s' is not a valid number",
                             p->operand);
                direct_val = 0;
            }
        }
    }
    if (!p->has_label)
    {
        if (is_set)
            report_error(line_no, "set needs a label on the same line");
        return;
    }
    if (!is_valid_label(p->label))
    {
        report_error(line_no, "bogus label name '%s'", p->label);
    }
    else if (find_label(p->label) >= 0)
    {
        report_error(line_no, "duplicate label '%s'", p->label);
    }
    else if (label_count >= MAX_labelS)
    {
        report_error(line_no, "too many labels");
    }
    else
    {
        strcpy(label_table[label_count].name, p->label);
        label_table[label_count].direct_val = direct_val;
        label_table[label_count].is_set = is_set;
        label_table[label_count].used = 0;
        label_table[label_count].line = line_no;
        label_count++;
    }
}

/* Runs one pass over the source file. In pass 1 line-level problems are
   reported; in pass 2 operands are resolved and code is generated, and
   problems that need the symbol table (undefined labels) are reported. */
static void run_pass(FILE *in, int pass)
{
    char line[MAX_LINE];
    char text[MAX_TEXT];
    ParsedLine p;
    const instruction *def;
    const char *msg;
    long address = 0, direct_val = 0;
    unsigned long word;
    int line_no = 0, was_label = 0, rc, c;
    size_t len;

    rewind(in);
    while (fgets(line, sizeof line, in) != NULL)
    {
        line_no++;
        len = strlen(line);
        if (len == MAX_LINE - 1 && line[len - 1] != '\n')
        {
            if (pass == 1)
                report_error(line_no, "line too long (limit %d characters)",
                             MAX_LINE - 2);
            while ((c = getc(in)) != '\n' && c != EOF)
                ; /* discard rest of the line */
            continue;
        }

        parse_line(line, &p);
        def = p.has_mnemonic ? find_instr(p.mnemonic) : NULL;

        if (pass == 1)
        {
            define_label(&p, def, line_no, address);
        }
        else if (p.has_label && is_valid_label(p.label) && (def == NULL || def->opcode != OP_set))
        {
            sprintf(text, "%s:", p.label);
            add_list(address, 0, 0UL, text);
        }

        if (!p.has_mnemonic)
            continue; /* blank, comment or label only */
        if (def == NULL)
        {
            if (pass == 1)
                report_error(line_no, "unknown mnemonic '%s'", p.mnemonic);
            continue;
        }

        msg = check_operands(def, &p);
        if (msg != NULL)
        {
            if (pass == 1)
                report_error(line_no, "%s", msg);
        }
        else if (pass == 1)
        {
            if (def->opcode != OP_set && address >= MAX_CODE)
                report_error(line_no, "program too large (%d words max)",
                             MAX_CODE);
        }
        else if (def->opcode == OP_set)
        {
            if (p.has_label)
            {
                sprintf(text, "%s: set %s", p.label, p.operand);
                add_list(-1L, 0, 0UL, text);
            }
        }
        else
        { /* pass 2, well-formed instruction */
            direct_val = 0;
            rc = 0;
            was_label = 0;
            if (def->kind != none)
                rc = resolve_operand(p.operand, &direct_val, &was_label);
            if (rc == 1)
            {
                report_error(line_no, "no such label '%s'", p.operand);
            }
            else if (rc == 2)
            {
                report_error(line_no, "'%s' is not a number", p.operand);
            }
            else if (rc == 3)
            {
                report_error(line_no, "number '%s' is out of range",
                             p.operand);
            }
            else
            {
                if (was_label && def->kind == offset)
                    direct_val -= address + 1; /* PC is already incremented */
                if (def->opcode >= 0)
                {
                    report_error(line_no,
                                 "operand %ld does not fit in 24 bits", direct_val);
                }
                else if (address < MAX_CODE)
                {
                    word = encode(def, direct_val);
                    code[address] = word;
                    make_text(text, def, &p, address, direct_val, was_label);
                    add_list(address, 1, word, text);
                }
            }
        }
        if (def->opcode != OP_set)
            address++; /* set occupies no memory word */
    }
    if (pass == 1)
        code_size = (address > MAX_CODE) ? MAX_CODE : address;
}

/* Warn about labels that are defined but never used. */
static void warn_unused_labels(void)
{
    int i;
    for (i = 0; i < label_count; i++)
        if (!label_table[i].used)
            report_warning(label_table[i].line,
                           "label '%s' defined but never used",
                           label_table[i].name);
}

/* ------------------------------------------------------------------ */
/* 7. Output files                                                     */
/* ------------------------------------------------------------------ */

/* Listing: "address word mnemonic operand", with label lines such as
   "00000002 start:" and set lines with a blank address column. */
static void write_listing(FILE *out)
{
    int i;
    const ListEntry *e;

    for (i = 0; i < list_count; i++)
    {
        e = &list_table[i];
        if (e->address < 0)
            fprintf(out, "%8s %s\n", "", e->text);
        else if (e->has_word)
            fprintf(out, "%08lX %08lX %s\n", (unsigned long)e->address,
                    e->word, e->text);
        else
            fprintf(out, "%08lX %s\n", (unsigned long)e->address, e->text);
    }
}

/* Object file: every word as 4 bytes, least significant byte first, so
   the file is the same on every machine. Returns 1 on success. */
static int write_object(const char *name)
{
    FILE *f = fopen(name, "wb");
    long i;
    unsigned long w;

    if (f == NULL)
        return 0;
    for (i = 0; i < code_size; i++)
    {
        w = code[i];
        putc((int)(w & 0xFFUL), f);
        putc((int)((w >> 8) & 0xFFUL), f);
        putc((int)((w >> 16) & 0xFFUL), f);
        putc((int)((w >> 24) & 0xFFUL), f);
    }
    return fclose(f) == 0;
}

/* Builds "<src without extension><ext>" into dst. */
static void make_output_name(const char *src, const char *ext,
                             char *dst, size_t size)
{
    const char *dot = strrchr(src, '.');
    const char *slash = strrchr(src, '/');
    size_t len;

    if (dot != NULL && slash != NULL && dot < slash)
        dot = NULL; /* the dot is in a directory */
    len = (dot != NULL) ? (size_t)(dot - src) : strlen(src);
    if (len + strlen(ext) + 1 > size)
        len = size - strlen(ext) - 1;
    memcpy(dst, src, len);
    strcpy(dst + len, ext);
}

int main(int argc, char *argv[])
{
    FILE *in, *lst;
    char lst_name[256], obj_name[256];

    if (argc != 2)
    {
        fprintf(stderr, "usage: %s file.asm\n", argv[0]);
        return 1;
    }
    in = fopen(argv[1], "r");
    if (in == NULL)
    {
        fprintf(stderr, "cannot open '%s'\n", argv[1]);
        return 1;
    }

    run_pass(in, 1);
    run_pass(in, 2);
    fclose(in);
    warn_unused_labels();
    print_diags();

    if (error_count > 0)
    {
        fprintf(stderr, "%d error(s), %d warning(s): no output produced\n",
                error_count, warning_count);
        return 1;
    }

    make_output_name(argv[1], ".lst", lst_name, sizeof lst_name);
    make_output_name(argv[1], ".o", obj_name, sizeof obj_name);
    lst = fopen(lst_name, "w");
    if (lst == NULL)
    {
        fprintf(stderr, "cannot create '%s'\n", lst_name);
        return 1;
    }
    write_listing(lst);
    fclose(lst);
    if (!write_object(obj_name))
    {
        fprintf(stderr, "cannot write '%s'\n", obj_name);
        return 1;
    }
    printf("assembled %ld words (%d warning(s)) -> %s, %s\n", code_size,
           warning_count, lst_name, obj_name);
    return 0;
}
