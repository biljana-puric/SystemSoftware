#include "stdbool.h"
#include "string.h"

typedef struct symbol_table
{
  char* label;
  char* section;
  int offset;
  int local;
  int num;
  struct symbol_table * next;
} Symbol_table;

typedef struct literal_pool {
    int value;
    char* symbol;
    char* section;
    int offset;
    int rb;
    struct literal_pool* next;
} LiteralPool;

typedef struct section_table
{
  char* label;
  int offset;
  int size;
  int my_location_counter;
  struct section_table * next;
  unsigned char* content;
  struct literal_pool *global_literal_pool;
} Section_table;

typedef struct relocation_table
{
  char* section;
  int offset;
  char* type;
  char* symbol;
  int addend;
  struct relocation_table * next;
} Reloc_table;



struct literal_pool* add_literal(struct literal_pool* lp, char*symbol, int value, char* section, int offset);
static bool first_pass = true;
struct symbol_table *global_symbol_table;
struct section_table *global_section_table;

struct section_table *current_section;
static int symbol_id = 0;
static int literal_id = 0;
struct relocation_table *global_reloc_table;

void add_symbol(char* label, char* section, int offset);
void add_section(char* label, int offset, int size);
void add_relocation(char* section, int offset, char*type, char* symbol, int addend);
int get_literal_id(int literal, char* symbol);
void global_directive_first_pass(char* arg);
void extern_directive_first_pass(char* arg);
void skip_directive_first_pass(char* arg);
void word_directive_first_pass(char* arg);
void section_directive_first_pass(char* arg);
void end_directive_first_pass(char* arg);

void label_first_pass(char* arg);
void label_second_pass(char* arg);

bool exists_in_sym_table(char* label);
bool exists_in_pool(char* label);

void global_directive_second_pass(char* arg);
void extern_directive_second_pass(char* arg);
void skip_directive_second_pass(char* arg);
void word_directive_second_pass(char* arg);
void section_directive_second_pass(char* arg);
void end_directive_second_pass(char* arg);

void halt();
void iint();
void iret();
void icall(char* operand);
void rret();
void jmp(char* operand);
void beq(char*a, char*b, char*operand);
void bne(char*a, char*b, char*operand);
void bgt(char*a, char*b, char*operand);
void push(char* b);
void pop(char* a);
void xchg(char* a, char* b);
void add(char* a, char* b);
void sub(char* a, char* b);
void mul(char* a, char* b);
void ddiv(char* a, char* b);
void not(char* a);
void and(char* a, char* b);
void or(char* a, char* b);
void xor(char* a, char* b);
void shl(char* a, char* b);
void shr(char* a, char* b);
void ld(char*operand, char*reg, char* op);
void st(char*operand, char*reg, char* op);
void instruction_cssrd(char* a, char* b);
void csrwr(char* a, char* b);