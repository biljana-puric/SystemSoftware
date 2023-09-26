#include "stdbool.h"
#include "string.h"

typedef struct sym_table
{
  char* file_name;
  char* label;
  char* section;
  int offset;
  int local;
  int num;
  struct sym_table * next;
} Sym_table;

typedef struct map
{
  int address;
  unsigned char content;
  struct map* next;
} Map;

typedef struct sect_table
{
  char* file_name;
  char* label;
  int offset;
  int size;
  int my_location_counter;
  struct sect_table * next;
  unsigned char* content;
  bool placed;
} Sect_table;

typedef struct reloc_table
{
  char* file_name;
  char* section;
  int offset;
  char* type;
  char* symbol;
  int addend;
  struct reloc_table * next;
} Rel_table;

struct sym_table *global_sym_table;
struct sym_table *my_symbol_table;
struct sect_table *global_sect_table;
struct reloc_table *global_rel_table;
struct map *global_map;
void checkSymbols();
void initialize(char** files, int num);
void mapping(int* place, char** section, int number);
void odredjivanje();
void razresavanje();
bool exists_in_symbol_table(char* label);
void add_my_symbols(char* file_name, char* label, char* section, int offset, int num, int local);
void add_symb(char* file_name, char* label, char* section, int offset, int num, int type);
void add_sect(char* file_name, char* label, int offset, int size, int my_lc);
void add_reloc(char* file_name, char* section, int offset, char*type, char* symbol, int addend);
void add_to_map(int address, unsigned char content);