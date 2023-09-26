#include "../inc/assembler.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include "../lexer.h"
#include "../parser.h"
#include "stdbool.h"
#define HALT_INSTRUCTION_CODE 0x00000000
#define INT_INSTRUCTION_CODE 0x10000000

char *output_file = NULL;

void get_in_content(int content){
  int num_elements = current_section->size;
  if(current_section->content==NULL) {
    current_section->content = (unsigned char*)malloc(num_elements * sizeof(unsigned char));
  }
  unsigned char byte1 = (content >> 24) & 0xFF;
  unsigned char byte2 = (content >> 16) & 0xFF;
  unsigned char byte3 = (content >> 8) & 0xFF;
  unsigned char byte4 = content & 0xFF;
  current_section->content[current_section->my_location_counter++] = byte4;
  current_section->content[current_section->my_location_counter++] = byte3;
  current_section->content[current_section->my_location_counter++] = byte2;
  current_section->content[current_section->my_location_counter++] = byte1;
}

bool exists_in_sym_table(char* label){
  for(struct symbol_table* current = global_symbol_table; current; current = current->next){
    if(strcmp(current->label, label)==0) return true;
  }
  return false;
}

bool exists_in_rel_table(char* label){
  for(struct relocation_table* current = global_reloc_table; current; current = current->next){
    if(strcmp(current->symbol, label)==0) return true;
  }
  return false;
}

bool exists_in_pool(char* label){
  for(struct literal_pool* lp = current_section->global_literal_pool; lp; lp = lp->next){
    if(lp->symbol!=NULL){
      if(strcmp(lp->symbol, label)==0) return true;
    }
  }
  return false;
}

int get_literal_id(int literal, char* symbol){
  if(literal == -1){
    for(struct literal_pool* lp = current_section->global_literal_pool; lp; lp=lp->next){
      if(lp->symbol!=NULL){
        if(strcmp(lp->symbol, symbol)==0){
          return lp->rb;
        }
      }
    }
  }
  else{
    for(struct literal_pool* lp = current_section->global_literal_pool; lp; lp=lp->next){
      if(lp->value == literal){
        return lp->rb;
      }
    }
  }
}

struct literal_pool* add_literal(struct literal_pool* lp, char*symbol, int value, char* section, int offset){
  if(symbol!=NULL){
    if(exists_in_pool(symbol)) return lp;
  }
  struct literal_pool* new_literal = (struct literal_pool*)malloc(sizeof(struct literal_pool));
  if (new_literal == NULL) {
      fprintf(stderr, "Memory allocation error for new rel.\n");
      exit(EXIT_FAILURE);
  }

  new_literal->value = value;

  new_literal->section = strdup(section);
  if(symbol!=NULL) {
    new_literal->symbol = strdup(symbol);
  }

  
  new_literal->next = NULL; 
  new_literal->rb = literal_id++;
  new_literal->offset = new_literal->rb *4 + current_section->size;
  if (lp == NULL) {
      lp = new_literal;
  } else {
      struct literal_pool* current = lp;
      while (current->next != NULL) {
          current = current->next;
      }
      current->next = new_literal;
  }
  return lp;
}

void add_relocation(char* section, int offset, char*type, char* symbol, int addend){
  if(exists_in_rel_table(symbol)) return;
  struct relocation_table* new_reloc = (struct relocation_table*)malloc(sizeof(struct relocation_table));
  if (new_reloc == NULL) {
      fprintf(stderr, "Memory allocation error for new rel.\n");
      exit(EXIT_FAILURE);
  }
  new_reloc->section = strdup(section);
  new_reloc->symbol = strdup(symbol);
  new_reloc->offset = offset;
  new_reloc->addend = addend; 
  new_reloc->type = type; 
  new_reloc->next = NULL; 

  if (global_reloc_table == NULL) {
      global_reloc_table = new_reloc;
  } else {
      struct relocation_table* current = global_reloc_table;
      while (current->next != NULL) {
          current = current->next;
      }
      current->next = new_reloc;
  }
}

void label_first_pass(char*arg){
  bool found_global = false;
  if(exists_in_sym_table(arg)){
    for(struct symbol_table* current = global_symbol_table; current; current = current->next){
      if(strcmp(current->label, arg)==0){
        if(strcmp(current->section, "0")==0){
          found_global = true;
          //current_section->global_literal_pool = add_literal(current_section->global_literal_pool, arg, current_section->my_location_counter*4+1, current_section->label, current_section->my_location_counter*4+1);
          current->section = strdup(current_section->label); //stavlja se za globalni simbol sekcija
          current->offset = current_section->my_location_counter;
        }
        //if(current->label == arg) current->offset = current->section->my_location_counter;
        //mora izgleda u symbol table section da bude pokazivac na section
        // mozda i ne mora jer moze da se pretrazi pa da se izmeni
      }
    }
    if(found_global == false){
      printf("Multiple symbol declaration!\n");
      return;
    }
  }
  else{
    add_symbol(arg, current_section->label, current_section->my_location_counter*4);
    //current_section->global_literal_pool = add_literal(current_section->global_literal_pool, arg, current_section->my_location_counter*4+1, current_section->label, current_section->my_location_counter*4+1);
    //proveriti ove pomeraje
  }
}
void label_second_pass(char*arg){
  //empty
}

void add_symbol(char* label, char* section, int offset){
  if(offset!=0) offset = (offset-1)/4;
  if(exists_in_sym_table(label)) return;
  struct symbol_table* new_symbol = (struct symbol_table*)malloc(sizeof(struct symbol_table));
  if (new_symbol == NULL) {
      fprintf(stderr, "Memory allocation error for new symbol.\n");
      exit(EXIT_FAILURE);
  }
  new_symbol->label = strdup(label);
  if (new_symbol->label == NULL) {
      fprintf(stderr, "Memory allocation error for symbol label.\n");
      free(new_symbol);
      exit(EXIT_FAILURE);
  }
  new_symbol->section = strdup(section);
  if (new_symbol->section == NULL) {
      fprintf(stderr, "Memory allocation error for symbol label.\n");
      free(new_symbol);
      exit(EXIT_FAILURE);
  }
  new_symbol->offset = offset; // location counter
  new_symbol->local = 1; 
  symbol_id++;
  new_symbol->num = symbol_id; 
  new_symbol->next = NULL; 

  if (global_symbol_table == NULL) {
      global_symbol_table = new_symbol;
  } else {
      struct symbol_table* current = global_symbol_table;
      while (current->next != NULL) {
          current = current->next;
      }
      current->next = new_symbol;
  }
}

void add_section(char* label, int offset, int size) {
    struct section_table* new_section = (struct section_table*)malloc(sizeof(struct section_table));
    if (new_section == NULL) {
        fprintf(stderr, "Memory allocation error for new section.\n");
        exit(EXIT_FAILURE);
    }
    new_section->label = strdup(label);
    if (new_section->label == NULL) {
        fprintf(stderr, "Memory allocation error for section label.\n");
        free(new_section);
        exit(EXIT_FAILURE);
    }

    new_section->offset = offset;
    new_section->size = size;
    new_section->next = NULL;
    new_section->my_location_counter = 0;

    if (global_section_table == NULL) {
        global_section_table = new_section;
    } else {
        struct section_table* current_sec = global_section_table;
        while (current_sec->next != NULL) {
            current_sec = current_sec->next;
        }
        current_sec->next = new_section;
    }
}

void global_directive_first_pass(char* arg){
  char *symbol = strtok(arg, ",");
  while (symbol != NULL) {
    while (*symbol == ' ') {
        symbol++;
    }
    char *end = symbol + strlen(symbol) - 1;
    while (*end == ' ' || *end == '\n' || *end == '\r') {
        *end = '\0';
        end--;
    }
    add_symbol(symbol, "0", 0); 
    symbol = strtok(NULL, ",");
  }
}

void extern_directive_first_pass(char* arg){
  char *symbol = strtok(arg, ",");
  
  while (symbol != NULL) {
    while (*symbol == ' ') {
        symbol++;
    }
    char *end = symbol + strlen(symbol) - 1;
    while (*end == ' ' || *end == '\n' || *end == '\r') {
        *end = '\0';
        end--;
    }
    add_symbol(symbol, "0", 0); 
    symbol = strtok(NULL, ",");
  }
}

void skip_directive_first_pass(char* arg){
  
}

void word_directive_first_pass(char* arg){
  char *symbol = strtok(arg, ",");
  
  while (symbol != NULL) {
    while (*symbol == ' ') {
        symbol++;
    }
    char *end = symbol + strlen(symbol) - 1;
    while (*end == ' ' || *end == '\n' || *end == '\r') {
        *end = '\0';
        end--;
    }
    
    current_section->my_location_counter+=4;
    symbol = strtok(NULL, ",");
  }
}

void section_directive_first_pass(char* arg){
  if(exists_in_sym_table(arg)){
    printf("Multiple symbol declaration: %s\n", arg);
    exit(-1);
  }
  add_symbol(arg, arg, 0); //jer se dodaje nova sekcija, ovo je offset od pocetka sekcije
  add_section(arg, 0, 0); //trebalo bi offset od pocetka fajla, ne znam kako da dodam ovde velicinu (u bajtovima)
  //size se doda kad se zavrsi sekcija - linija koda ispod
  if(current_section!=NULL) {
    current_section->size = current_section->my_location_counter;
    for(struct section_table* current = global_section_table; current; current=current->next){
      if(strcmp(current->label, arg)==0){
        current_section = current; //postavi se nova sekcija kao trenutna
      }
    }
  }
  else{
    current_section = global_section_table;
  }
}

void end_directive_first_pass(char* arg){
  current_section->size = current_section->my_location_counter;
  // printf("\n");
  // for(struct symbol_table* current = global_symbol_table; current; current=current->next){
  //   printf("Symbol name: %s | Section name: %s | Offset: %d | Redni broj: %d \n", 
  //   current->label, current->section, current->offset, current->num);
  //   // for(int i=0; i<current->my_location_counter; i++){
  //   //   printf("%d ", current->content[i]);
  //   // }
  // }
  // printf("Sekcije:\n");
  // for(struct section_table* current = global_section_table; current; current=current->next){
  //   printf("Section name: %s | LC: %d | Offset: %d | Size: %d \n", 
  //     current->label, current->my_location_counter, current->offset, current->size);
  //   // for(int i=0; i<current->my_location_counter; i++){
  //   //   if(current->content[i]) printf("%d ", current->content[i]);
  //   // }
  // }
  for(struct section_table* current = global_section_table; current; current = current->next){
    current->my_location_counter = 0;
  }
  first_pass = false;
}

void global_directive_second_pass(char* arg){
  char *symbol = strtok(arg, ",");

  while (symbol != NULL) {
    while (*symbol == ' ') {
        symbol++;
    }
    char *end = symbol + strlen(symbol) - 1;
    while (*end == ' ' || *end == '\n' || *end == '\r') {
        *end = '\0';
        end--;
    }
    for(struct symbol_table* current = global_symbol_table; current; current = current->next){
      if(strcmp(current->label, symbol)==0){
        current->local = 2; //postavi da je simbol globalan
      }
    }
    
    symbol = strtok(NULL, ",");
  }
  
}
void extern_directive_second_pass(char* arg){
  char *symbol = strtok(arg, ",");
  
  while (symbol != NULL) {
    while (*symbol == ' ') {
        symbol++;
    }
    char *end = symbol + strlen(symbol) - 1;
    while (*end == ' ' || *end == '\n' || *end == '\r') {
        *end = '\0';
        end--;
    }
    for(struct symbol_table* current = global_symbol_table; current; current = current->next){
      if(strcmp(current->label, symbol)==0){
        current->local = 3; //postavi da je simbol extern
      }
    }
    symbol = strtok(NULL, ",");
  }
}
void skip_directive_second_pass(char* arg){
  int num_elements = current_section->size;
  if(current_section->content==NULL) {
    current_section->content = (unsigned char*)malloc(num_elements * sizeof(unsigned char));
  }
  int skip = atoi(arg);
  for(int i=0; i<skip; i++){
    current_section->content[current_section->my_location_counter++] = 0;
  }
}

void word_directive_second_pass(char* arg){
  char *symbol = strtok(arg, ",");
  
  while (symbol != NULL) {
    while (*symbol == ' ') {
        symbol++;
    }
    char *end = symbol + strlen(symbol) - 1;
    while (*end == ' ' || *end == '\n' || *end == '\r') {
        *end = '\0';
        end--;
    }
    char *endptr;
    long num1 = strtol(symbol, &endptr, 10);
    if (errno == EINVAL || *endptr != '\0') {
      long num1 = strtol(symbol, &endptr, 16);
      if (errno == EINVAL || *endptr != '\0'){
        //simbol je
        if(!exists_in_sym_table(symbol)){
          add_symbol(symbol, current_section->label, current_section->my_location_counter*4+1); //treba drugi offset
          add_relocation(current_section->label, current_section->my_location_counter, "pc", symbol, -4);
          get_in_content(0x00000000);
        }
        else{
          for(struct symbol_table* current = global_symbol_table; current; current = current->next){
            if(strcmp(current->label, symbol)==0){
              current->offset = current_section->my_location_counter;
            }
          }
        }
      }
      else{
        //heksa
        get_in_content(num1);
      }
    }
    else{
      //dec literal
      get_in_content(num1);
    }
    symbol = strtok(NULL, ",");
  }
}

void section_directive_second_pass(char* arg){
  for(struct section_table* current = global_section_table; current; current=current->next){
    if(strcmp(current->label, arg)==0){
      current_section = current;
    }
  }
}

void end_directive_second_pass(char* arg){
  current_section->size = current_section->my_location_counter;
  //printf("%d\n", current_section->my_location_counter);
  //treba da se zavrsi asembliranje
  //printf("Trenutna: %s", current_section->label);
  // for(struct symbol_table* current = global_symbol_table; current; current=current->next){
  //   printf("Symbol name: %s | Section name: %s | Offset: %d | Type: %d | Redni broj: %d \n", 
  //   current->label, current->section, current->offset, current->local, current->num);
  //   // for(int i=0; i<current->my_location_counter; i++){
  //   //   printf("%d ", current->content[i]);
  //   // }
  // }
  
  FILE* binFile = fopen(output_file, "wb");
  if (!binFile) {
      perror("Nemoguće otvoriti fajl za pisanje");
      return;
  }
  
  int size=0; //broj sekcija koje treba linker da procita
  for(struct section_table* current = global_section_table; current; current=current->next){
    size++;
    for(struct literal_pool* lp = current->global_literal_pool; lp; lp=lp->next){
      current->size += 4;
    }
  }
  int sizesym=0; //broj simbola
  for(struct symbol_table* symbol = global_symbol_table; symbol; symbol=symbol->next){
    sizesym++;
  }
  for(struct section_table* current = global_section_table; current; current=current->next){
    printf("Section name: %s | LC: %d | Offset: %d | Size: %d | Content: ", 
      current->label, current->my_location_counter, current->offset, current->size);
    for(int i=0; i<current->size; i++){
      printf("%02x ", current->content[i]);
    }
    printf("\n");
  }
  //printf("%d, %d\n", sizesym, size);
  fwrite(&size, sizeof(int), 1, binFile);
  fwrite(&sizesym, sizeof(int), 1, binFile);
  for(struct section_table* current = global_section_table; current; current=current->next){
    fwrite(current->label, sizeof(char), 16, binFile);
    fwrite(&current->offset, sizeof(int), 1, binFile);
    fwrite(&current->my_location_counter, sizeof(int), 1, binFile);
    fwrite(&current->size, sizeof(int), 1, binFile);
    for(int i=0; i<current->my_location_counter; i++){
      fwrite(&current->content[i], sizeof(char), 1, binFile);
    }
    for(struct literal_pool* lp = current->global_literal_pool; lp; lp=lp->next){
      if(lp->symbol==NULL) fwrite(&lp->value, sizeof(int), 1, binFile);
      else {
        //relokacije
        int nula = 0;
        fwrite(&nula, sizeof(int), 1, binFile);
      }
      //printf("%08x\n", lp->value);
      // if(lp->symbol!=NULL){
      //   printf("%x | %s | %s | %d\n", lp->value, lp->symbol, lp->section, lp->offset);
      // }
      // else printf("%x | %s | %d\n", lp->value, lp->section, lp->offset);
    }
  }
  printf("Relokacioni zapisi: \n");
  for(struct relocation_table* curr = global_reloc_table; curr; curr= curr->next){
    printf("Reloc zapis: %s | %s | %d | %s | %d\n", curr->symbol, curr->section, curr->offset, curr->type, 
    curr->addend);
  }
  
  for(struct symbol_table* symbol = global_symbol_table; symbol; symbol=symbol->next){
    fwrite(symbol->label, sizeof(char), 16, binFile);
    fwrite(symbol->section, sizeof(char), 16, binFile);
    fwrite(&symbol->offset, sizeof(int), 1, binFile);
    fwrite(&symbol->local, sizeof(int), 1, binFile);
    fwrite(&symbol->num, sizeof(int), 1, binFile);
  }
  
  for(struct relocation_table* curr = global_reloc_table; curr; curr= curr->next){
    fwrite(curr->symbol, sizeof(char), 16, binFile);
    fwrite(curr->section, sizeof(char), 16, binFile);
    fwrite(&curr->offset, sizeof(int), 1, binFile);
    fwrite(curr->type, sizeof(char), 4, binFile);
    fwrite(&curr->addend, sizeof(int), 1, binFile);
  }

  fclose(binFile);
}

void halt(){
  get_in_content(0x00000000);
  
}

void iint(){
  get_in_content(0x10000000);
}

void iret(){
  get_in_content(0x970e0004);
  get_in_content(0x93fe0004);
}

void rret(){
  get_in_content(0x93fe0004);
}

void jmp(char* operand){
  if (operand[strlen(operand) - 1] == '\n') {
        operand[strlen(operand) - 1] = '\0';
    }
  char CODE[13];
  int off;
  int defined = 0;
  if(exists_in_sym_table(operand)){
    for(struct symbol_table* current = global_symbol_table; current; current = current->next){
      if(strcmp(current->label, operand)==0){
        defined = current->local;
      }
    }
  }
  if(exists_in_sym_table(operand) && defined==1){
    //znaci skace se unutar sekcije
    strcpy(CODE, "0x30f00000");
    for(struct symbol_table* current = global_symbol_table; current; current = current->next){
      if(strcmp(current->label,operand)==0){
        off = current->offset;
      }
    }
    if (current_section->my_location_counter > off) {
      //skok unazad
      int pomeraj = current_section->my_location_counter - off;
      if(pomeraj<4096) {
        //pomeraj je manji od 12 bita
        off = off - current_section->my_location_counter + 1;
        off = off & 0xFFF;
      }
      else{
        //veci od 12 bita
        //ide u bazen
        current_section->global_literal_pool = add_literal(current_section->global_literal_pool, operand, 0, current_section->label, current_section->my_location_counter*4+1);
        strcpy(CODE, "0x38f00000");
        int pomeraj_do_bazena = current_section->size + get_literal_id(-1, operand) *4 - (current_section->my_location_counter);
        int JMP_INSTRUCTION_CODE;
        sscanf(CODE, "%x", &JMP_INSTRUCTION_CODE);
        JMP_INSTRUCTION_CODE += pomeraj_do_bazena;
        get_in_content(JMP_INSTRUCTION_CODE);
        return;
      }
    }
    else{
      //skok unapred
      int pomeraj = off - current_section->my_location_counter;
      if(pomeraj<4096) {
        off = off - current_section->my_location_counter +1;
      }
      else{
        //veci od 12 bita
        //ide u bazen
        current_section->global_literal_pool = add_literal(current_section->global_literal_pool, operand, 0, current_section->label, current_section->my_location_counter+1);
        strcpy(CODE, "0x38f00000");
        int pomeraj_do_bazena = current_section->size + get_literal_id(-1, operand) *4 - (current_section->my_location_counter);
        int JMP_INSTRUCTION_CODE;
        sscanf(CODE, "%x", &JMP_INSTRUCTION_CODE);
        JMP_INSTRUCTION_CODE += pomeraj_do_bazena;
        get_in_content(JMP_INSTRUCTION_CODE);
        return;
      }
    }
    int JMP_INSTRUCTION_CODE;
    sscanf(CODE, "%x", &JMP_INSTRUCTION_CODE);
    JMP_INSTRUCTION_CODE += off;
    get_in_content(JMP_INSTRUCTION_CODE);
  }
  else{
    char *endptr;
    long num1 = strtol(operand, &endptr, 10); 
    if (errno == EINVAL || *endptr != '\0') { // ne treba num1 u add_literal jer je simbol
      if(defined!=1) current_section->global_literal_pool = add_literal(current_section->global_literal_pool, operand, num1, current_section->label, current_section->my_location_counter+1);
      if(defined!=1) add_symbol(operand, current_section->label, current_section->my_location_counter*4+1);
      strcpy(CODE, "0x38f00000");
      int pomeraj_do_bazena = current_section->size + get_literal_id(-1, operand) *4 - (current_section->my_location_counter);
      int JMP_INSTRUCTION_CODE;
      sscanf(CODE, "%x", &JMP_INSTRUCTION_CODE);
      JMP_INSTRUCTION_CODE += pomeraj_do_bazena;
      get_in_content(JMP_INSTRUCTION_CODE);
      //je simbol treba relok zapis
      add_relocation(current_section->label, current_section->size + get_literal_id(-1, operand)*4, "pc", operand, -4);
    } 
    else {
      if(num1 > 4095){
        current_section->global_literal_pool = add_literal(current_section->global_literal_pool, NULL, num1, current_section->label, current_section->my_location_counter*4+1);
        strcpy(CODE, "0x30f00000");
        int pomeraj_do_bazena = current_section->size + get_literal_id(num1, NULL) *4 - (current_section->my_location_counter);
        int JMP_INSTRUCTION_CODE;
        sscanf(CODE, "%x", &JMP_INSTRUCTION_CODE);
        JMP_INSTRUCTION_CODE+=pomeraj_do_bazena;
        get_in_content(JMP_INSTRUCTION_CODE);
      }
      else{
        strcpy(CODE, "0x30000000");
        int JMP_INSTRUCTION_CODE;
        sscanf(CODE, "%x", &JMP_INSTRUCTION_CODE);
        JMP_INSTRUCTION_CODE += num1;
        get_in_content(JMP_INSTRUCTION_CODE);
      }
      //je literal
    }
  }
}

void beq(char*a, char*b, char*operand){
  if (a[strlen(a) - 1] == '\n') {
        a[strlen(a) - 1] = '\0';
    }
  if (b[strlen(b) - 1] == '\n') {
      b[strlen(b) - 1] = '\0';
  }
  if (operand[strlen(operand) - 1] == '\n') {
      operand[strlen(operand) - 1] = '\0';
  }
  char CODE[13]; // +1 za null terminator
  int off;
  char areg[2];
  char breg[2];
  sscanf(a, "%[^,]", areg);
  sscanf(b, "%[^,]", breg);
  int defined = 0;
  if(exists_in_sym_table(operand)){
    for(struct symbol_table* current = global_symbol_table; current; current = current->next){
      if(strcmp(current->label, operand)==0){
        defined = current->local;
      }
    }
  }
  if(exists_in_sym_table(operand) && defined==1){
    printf("ovde\n");
    //znaci skace se unutar sekcije
    strcpy(CODE, "0x31f");
    strcat(CODE, breg);
    strcat(CODE, areg);
    strcat(CODE, "000");
    for(struct symbol_table* current = global_symbol_table; current; current = current->next){
      if(strcmp(current->label,operand)==0){
        off = current->offset;
      }
    }
    if (current_section->my_location_counter > off) {
      //skok unazad
      int pomeraj = current_section->my_location_counter - off;
      if(pomeraj<4095) {
        //pomeraj je manji od 12 bita
        off = off - current_section->my_location_counter +1;
        off = off & 0xFFF;
      }
      else{
        //veci od 12 bita ???????
        //ide u bazen
        current_section->global_literal_pool = add_literal(current_section->global_literal_pool, operand, 0, current_section->label, current_section->my_location_counter*4+1);
        strcpy(CODE, "0x31f");
        strcat(CODE, breg);
        strcat(CODE, areg);
        strcat(CODE, "000");
        int pomeraj_do_bazena = current_section->size + get_literal_id(-1, operand) *4 - (current_section->my_location_counter);
        int BEQ_INSTRUCTION_CODE;
        sscanf(CODE, "%x", &BEQ_INSTRUCTION_CODE);
        BEQ_INSTRUCTION_CODE += pomeraj_do_bazena;
        get_in_content(BEQ_INSTRUCTION_CODE);
        return;
      }
    }
    else{
      //skok unapred
      int pomeraj = off - current_section->my_location_counter;
      if(pomeraj<4095) {
        off = off - current_section->my_location_counter +1;
      }
      else{
        //veci od 12 bita ???????
        //ide u bazen
        current_section->global_literal_pool = add_literal(current_section->global_literal_pool, operand, 0, current_section->label, current_section->my_location_counter*4+1);
        strcpy(CODE, "0x31f");
        strcat(CODE, breg);
        strcat(CODE, areg);
        strcat(CODE, "000");
        int pomeraj_do_bazena = current_section->size + get_literal_id(-1, operand) *4 - (current_section->my_location_counter);
        int BEQ_INSTRUCTION_CODE;
        sscanf(CODE, "%x", &BEQ_INSTRUCTION_CODE);
        BEQ_INSTRUCTION_CODE += pomeraj_do_bazena;
        get_in_content(BEQ_INSTRUCTION_CODE);
        return;
      }
    }
    int BEQ_INSTRUCTION_CODE;
    sscanf(CODE, "%x", &BEQ_INSTRUCTION_CODE);
    BEQ_INSTRUCTION_CODE += off;
    get_in_content(BEQ_INSTRUCTION_CODE);
  }
  else{
    char *endptr;
    long num1 = strtol(operand, &endptr, 10); // 10 specifies base 10 (decimal)
    if (errno == EINVAL || *endptr != '\0') {
      current_section->global_literal_pool = add_literal(current_section->global_literal_pool, operand, num1, current_section->label, current_section->my_location_counter*4+1);
      add_symbol(operand, current_section->label, current_section->my_location_counter*4+1);
      strcpy(CODE, "0x39f");
      strcat(CODE, breg);
      strcat(CODE, areg);
      strcat(CODE, "000");
      int pomeraj_do_bazena = current_section->size + get_literal_id(-1, operand) *4 - (current_section->my_location_counter);
      int BEQ_INSTRUCTION_CODE;
      sscanf(CODE, "%x", &BEQ_INSTRUCTION_CODE);
      BEQ_INSTRUCTION_CODE += pomeraj_do_bazena;
      get_in_content(BEQ_INSTRUCTION_CODE);
      //je simbol treba relok zapis
      //printf("%s is not a valid number\n", str1);
      add_relocation(current_section->label, current_section->size + get_literal_id(-1, operand) *4, "pc", operand, -4);
    } 
    else {
      if(num1 > 4095){
        current_section->global_literal_pool = add_literal(current_section->global_literal_pool, NULL, num1, current_section->label, current_section->my_location_counter*4+1);
        //ne treba praviti relok zapis za literale
        strcpy(CODE, "0x31f");
        strcat(CODE, breg);
        strcat(CODE, areg);
        strcat(CODE, "000");
        int pomeraj_do_bazena = current_section->size + get_literal_id(num1, NULL) *4 - (current_section->my_location_counter);
        int BEQ_INSTRUCTION_CODE;
        sscanf(CODE, "%x", &BEQ_INSTRUCTION_CODE);
        BEQ_INSTRUCTION_CODE+=pomeraj_do_bazena;
        get_in_content(BEQ_INSTRUCTION_CODE);
      }
      else{
        strcpy(CODE, "0x31f");
        strcat(CODE, breg);
        strcat(CODE, areg);
        strcat(CODE, "000");
        int BEQ_INSTRUCTION_CODE;
        sscanf(CODE, "%x", &BEQ_INSTRUCTION_CODE);
        BEQ_INSTRUCTION_CODE += num1;
        get_in_content(BEQ_INSTRUCTION_CODE);
      }
      //je literal
      //printf("%s is a valid number: %ld\n", str1, num1);
    }
  }
}

void bne(char*a, char*b, char*operand){
  if (a[strlen(a) - 1] == '\n') {
        a[strlen(a) - 1] = '\0';
    }
  if (b[strlen(b) - 1] == '\n') {
      b[strlen(b) - 1] = '\0';
  }
  if (operand[strlen(operand) - 1] == '\n') {
      operand[strlen(operand) - 1] = '\0';
  }
  char CODE[13]; // +1 za null terminator
  int off;
  char areg[2];
  char breg[2];
  strncpy(areg, a, 1);
  strncpy(breg, b, 1);
  int defined = 0;
  if(exists_in_sym_table(operand)){
    for(struct symbol_table* current = global_symbol_table; current; current = current->next){
      if(strcmp(current->label, operand)==0){
        defined = current->local;
      }
    }
  }
  if(exists_in_sym_table(operand) && defined==1){
    //znaci skace se unutar sekcije
    strcpy(CODE, "0x32f");
    strcat(CODE, breg);
    strcat(CODE, areg);
    strcat(CODE, "000");
    for(struct symbol_table* current = global_symbol_table; current; current = current->next){
      if(strcmp(current->label,operand)==0){
        off = current->offset;
      }
    }
    off = off - current_section->my_location_counter + 1;
    int BNE_INSTRUCTION_CODE;
    sscanf(CODE, "%x", &BNE_INSTRUCTION_CODE);
    BNE_INSTRUCTION_CODE += off;
    get_in_content(BNE_INSTRUCTION_CODE);
  }
  else{
    char *endptr;
    long num1 = strtol(operand, &endptr, 10); // 10 specifies base 10 (decimal)
    if (errno == EINVAL || *endptr != '\0') {
      if(defined!=1) current_section->global_literal_pool = add_literal(current_section->global_literal_pool, operand, num1, current_section->label, current_section->my_location_counter*4+1);
      if(defined!=1) add_symbol(operand, current_section->label, current_section->my_location_counter*4+1);
      strcpy(CODE, "0x3af");
      strcat(CODE, breg);
      strcat(CODE, areg);
      strcat(CODE, "000");
      int pomeraj_do_bazena = current_section->size + get_literal_id(-1, operand) *4 - (current_section->my_location_counter);
      int BNE_INSTRUCTION_CODE;
      sscanf(CODE, "%x", &BNE_INSTRUCTION_CODE);
      BNE_INSTRUCTION_CODE += pomeraj_do_bazena;
      get_in_content(BNE_INSTRUCTION_CODE);
      //je simbol treba relok zapis
      //printf("%s is not a valid number\n", str1);
      add_relocation(current_section->label, current_section->size + get_literal_id(-1, operand) *4, "pc", operand, -4);
    } 
    else {
      if(num1 > 4095){
        current_section->global_literal_pool = add_literal(current_section->global_literal_pool, NULL, num1, current_section->label, current_section->my_location_counter*4+1);
        //ne treba praviti relok zapis za literale
        strcpy(CODE, "0x32f");
        strcat(CODE, breg);
        strcat(CODE, areg);
        strcat(CODE, "000");
        int pomeraj_do_bazena = current_section->size + get_literal_id(num1, NULL) *4 - (current_section->my_location_counter);
        int BNE_INSTRUCTION_CODE;
        sscanf(CODE, "%x", &BNE_INSTRUCTION_CODE);
        BNE_INSTRUCTION_CODE+=pomeraj_do_bazena;
        get_in_content(BNE_INSTRUCTION_CODE);
      }
      else{
        strcpy(CODE, "0x32f");
        strcat(CODE, breg);
        strcat(CODE, areg);
        strcat(CODE, "000");
        int BNE_INSTRUCTION_CODE;
        sscanf(CODE, "%x", &BNE_INSTRUCTION_CODE);
        BNE_INSTRUCTION_CODE += num1;
        get_in_content(BNE_INSTRUCTION_CODE);
      }
      //je literal
      //printf("%s is a valid number: %ld\n", str1, num1);
    }
  }
}
void bgt(char*a, char*b, char*operand){
  if (a[strlen(a) - 1] == '\n') {
        a[strlen(a) - 1] = '\0';
    }
  if (b[strlen(b) - 1] == '\n') {
      b[strlen(b) - 1] = '\0';
  }
  if (operand[strlen(operand) - 1] == '\n') {
      operand[strlen(operand) - 1] = '\0';
  }
  char CODE[13]; // +1 za null terminator
  int off;
  char areg[2];
  char breg[2];
  strncpy(areg, a, 1);
  strncpy(breg, b, 1);
  int defined = 0;
  if(exists_in_sym_table(operand)){
    for(struct symbol_table* current = global_symbol_table; current; current = current->next){
      if(strcmp(current->label, operand)==0){
        defined = current->local;
      }
    }
  }
  if(exists_in_sym_table(operand) && defined==1){
    //znaci skace se unutar sekcije
    strcpy(CODE, "0x33f");
    strcat(CODE, breg);
    strcat(CODE, areg);
    strcat(CODE, "000");
    for(struct symbol_table* current = global_symbol_table; current; current = current->next){
      if(strcmp(current->label,operand)==0){
        off = current->offset;
      }
    }
    //off = off - current_section->my_location_counter*4;
    int BGT_INSTRUCTION_CODE;
    sscanf(CODE, "%x", &BGT_INSTRUCTION_CODE);
    BGT_INSTRUCTION_CODE += off;
    get_in_content(BGT_INSTRUCTION_CODE);
  }
  else{
    char *endptr;
    long num1 = strtol(operand, &endptr, 10); // 10 specifies base 10 (decimal)
    if (errno == EINVAL || *endptr != '\0') {
      if(defined!=1) current_section->global_literal_pool = add_literal(current_section->global_literal_pool, operand, num1, current_section->label, current_section->my_location_counter*4+1);
      if(defined!=1) add_symbol(operand, current_section->label, current_section->my_location_counter*4+1);
      strcpy(CODE, "0x3bf");
      strcat(CODE, breg);
      strcat(CODE, areg);
      strcat(CODE, "000");
      int pomeraj_do_bazena = current_section->size + get_literal_id(-1, operand) *4 - (current_section->my_location_counter);
      //strcat(CODE, "") treba dodati pomeraj do bazena?????
      int BGT_INSTRUCTION_CODE;
      sscanf(CODE, "%x", &BGT_INSTRUCTION_CODE);
      BGT_INSTRUCTION_CODE += pomeraj_do_bazena;
      get_in_content(BGT_INSTRUCTION_CODE);
      //je simbol treba relok zapis
      //printf("%s is not a valid number\n", str1);
      add_relocation(current_section->label, current_section->size + get_literal_id(-1, operand) *4, "pc", operand, -4);
    } 
    else {
      if(num1 > 4095){
        current_section->global_literal_pool = add_literal(current_section->global_literal_pool, NULL, num1, current_section->label, current_section->my_location_counter*4+1);
        //ne treba praviti relok zapis za literale
        strcpy(CODE, "0x33f");
        strcat(CODE, breg);
        strcat(CODE, areg);
        strcat(CODE, "000");
        int pomeraj_do_bazena = current_section->size + get_literal_id(num1, NULL) *4 - (current_section->my_location_counter);
        //strcat(CODE, "") treba dodati pomeraj do bazena?????
        int BGT_INSTRUCTION_CODE;
        sscanf(CODE, "%x", &BGT_INSTRUCTION_CODE);
        BGT_INSTRUCTION_CODE+=pomeraj_do_bazena;
        get_in_content(BGT_INSTRUCTION_CODE);
      }
      else{
        strcpy(CODE, "0x33f");
        strcat(CODE, breg);
        strcat(CODE, areg);
        strcat(CODE, "000");
        int BGT_INSTRUCTION_CODE;
        sscanf(CODE, "%x", &BGT_INSTRUCTION_CODE);
        BGT_INSTRUCTION_CODE += num1;
        get_in_content(BGT_INSTRUCTION_CODE);
      }
      //je literal
      //printf("%s is a valid number: %ld\n", str1, num1);
    }
  }
}

void icall(char* operand){
  if (operand[strlen(operand) - 1] == '\n') {
      operand[strlen(operand) - 1] = '\0';
  }
  char CODE[13];
  int off;
  int defined = 0;
  for(struct symbol_table* current = global_symbol_table; current; current = current->next){
    if(strcmp(current->label, operand)==0){
      defined = current->local;
    }
  }
  if(exists_in_sym_table(operand)){
    if(defined==1){
      //postoji u tabeli simbola i definisan je
      strcpy(CODE, "0x20000000");
      for(struct symbol_table* current = global_symbol_table; current; current = current->next){
        if(strcmp(current->label,operand)==0){
          off = current->offset;
        }
      }
      if (current_section->my_location_counter > off) {
      //skok unazad
        int pomeraj = current_section->my_location_counter - off;
        if(pomeraj<4096) {
          //pomeraj je manji od 12 bita
          off = off - current_section->my_location_counter + 1;
          off = off & 0xFFF;
        }
        else{
          //veci od 12 bita 
          //ide u bazen
          current_section->global_literal_pool = add_literal(current_section->global_literal_pool, operand, 0, current_section->label, current_section->my_location_counter*4+1);
          strcpy(CODE, "0x21f00000");
          int pomeraj_do_bazena = current_section->size + get_literal_id(-1, operand) *4 - (current_section->my_location_counter);
          int CALL_INSTRUCTION_CODE;
          sscanf(CODE, "%x", &CALL_INSTRUCTION_CODE);
          CALL_INSTRUCTION_CODE += pomeraj_do_bazena;
          get_in_content(CALL_INSTRUCTION_CODE);
          return;
        }
      }
      else{
        //skok unapred
        int pomeraj = off - current_section->my_location_counter;
        if(pomeraj<4096) {
          off = off - current_section->my_location_counter + 1;
        }
        else{
          //veci od 12 bita 
          //ide u bazen
          current_section->global_literal_pool = add_literal(current_section->global_literal_pool, operand, 0, current_section->label, current_section->my_location_counter*4+1);
          strcpy(CODE, "0x21f00000");
          int pomeraj_do_bazena = current_section->size + get_literal_id(-1, operand) *4 - (current_section->my_location_counter);
          int CALL_INSTRUCTION_CODE;
          sscanf(CODE, "%x", &CALL_INSTRUCTION_CODE);
          CALL_INSTRUCTION_CODE += pomeraj_do_bazena;
          get_in_content(CALL_INSTRUCTION_CODE);
          return;
        }
      }
    }
    else{
      //nije definisan ali postoji u tabeli simbola znaci da je globalan
      strcpy(CODE, "0x21f00000");
      current_section->global_literal_pool = add_literal(current_section->global_literal_pool, operand, 0, current_section->label, current_section->my_location_counter*4+1);
      int pomeraj_do_bazena = current_section->size + get_literal_id(-1, operand) *4 - (current_section->my_location_counter);
      int CALL_INSTRUCTION_CODE;
      sscanf(CODE, "%x", &CALL_INSTRUCTION_CODE);
      CALL_INSTRUCTION_CODE+=pomeraj_do_bazena;
      get_in_content(CALL_INSTRUCTION_CODE);
      add_relocation(current_section->label, current_section->size + get_literal_id(-1, operand) *4, "pc", operand, -4);
    }
  }
  else{
    //ne postoji u tabeli simbola
    char *endptr;
    long num1 = strtol(operand, &endptr, 10); // 10 specifies base 10 (decimal)
    if (errno == EINVAL || *endptr != '\0') {
      //nije decimalan broj
      long num1 = strtol(operand, &endptr, 16);
      if (errno == EINVAL || *endptr != '\0') {
        //nije ni heksa broj
        // znaci simbol treba sve
        current_section->global_literal_pool = add_literal(current_section->global_literal_pool, operand, num1, current_section->label, current_section->my_location_counter*4+1);
        add_symbol(operand, current_section->label, current_section->my_location_counter*4+1);
        strcpy(CODE, "0x21f00000");
        int pomeraj_do_bazena = current_section->size + get_literal_id(-1, operand) *4 - (current_section->my_location_counter);
        int CALL_INSTRUCTION_CODE;
        printf("%s\n", operand);
        sscanf(CODE, "%x", &CALL_INSTRUCTION_CODE);
        CALL_INSTRUCTION_CODE += pomeraj_do_bazena;
        get_in_content(CALL_INSTRUCTION_CODE);
        add_relocation(current_section->label, current_section->size + get_literal_id(-1, operand) *4, "pc", operand, -4);
      }
      else{
        // u pitanju je heksa literal treba ga dodati u bazen
        if(num1 > 4095){
          current_section->global_literal_pool = add_literal(current_section->global_literal_pool, NULL, num1, current_section->label, current_section->my_location_counter*4+1);
          //ne treba praviti relok zapis za literale
          strcpy(CODE, "0x21f00000");
          int pomeraj_do_bazena = current_section->size + get_literal_id(num1, NULL) *4 - (current_section->my_location_counter);
          int CALL_INSTRUCTION_CODE;
          sscanf(CODE, "%x", &CALL_INSTRUCTION_CODE);
          CALL_INSTRUCTION_CODE+=pomeraj_do_bazena;
          get_in_content(CALL_INSTRUCTION_CODE);
        }
        else{
          strcpy(CODE, "0x20000000");
          int CALL_INSTRUCTION_CODE;
          sscanf(CODE, "%x", &CALL_INSTRUCTION_CODE);
          CALL_INSTRUCTION_CODE += num1;
          get_in_content(CALL_INSTRUCTION_CODE);
        }
        //je literal
      }
    }
    else{
      //decimalan literal, treba ga dodati u bazen
      if(num1 > 4095){
        current_section->global_literal_pool = add_literal(current_section->global_literal_pool, NULL, num1, current_section->label, current_section->my_location_counter*4+1);
        strcpy(CODE, "0x21f00000");
        int pomeraj_do_bazena = current_section->size + get_literal_id(num1, NULL) *4 - (current_section->my_location_counter);
        int CALL_INSTRUCTION_CODE;
        sscanf(CODE, "%x", &CALL_INSTRUCTION_CODE);
        CALL_INSTRUCTION_CODE+=pomeraj_do_bazena;
        get_in_content(CALL_INSTRUCTION_CODE);
      }
      else{
        strcpy(CODE, "0x20000000");
        int CALL_INSTRUCTION_CODE;
        sscanf(CODE, "%x", &CALL_INSTRUCTION_CODE);
        CALL_INSTRUCTION_CODE += num1;
        get_in_content(CALL_INSTRUCTION_CODE);
      }
    }
  }
  
}

void push(char* b){
  if (b[strlen(b) - 1] == '\n') {
    b[strlen(b) - 1] = '\0';
  }
  unsigned int hex_value;
  sscanf(b, "%d", &hex_value);
  char r[8];
  sprintf(r, "%x", hex_value);
  char CODE[13];
  strcpy(CODE, "0x81e0");
  strcat(CODE, r);
  strcat(CODE, "ffc");
  unsigned int PUSH_INSTRUCTION_CODE;
  sscanf(CODE, "%x", &PUSH_INSTRUCTION_CODE);
  get_in_content(PUSH_INSTRUCTION_CODE);
}

void pop(char* a){
  if (a[strlen(a) - 1] == '\n') {
        a[strlen(a) - 1] = '\0';
    }
  unsigned int hex_value;
  sscanf(a, "%d", &hex_value);
  char r[8];
  sprintf(r, "%x", hex_value);
  char CODE[13];
  strcpy(CODE, "0x93");
  strcat(CODE, r);
  strcat(CODE, "e0004");
  unsigned int POP_INSTRUCTION_CODE;
  sscanf(CODE, "%x", &POP_INSTRUCTION_CODE);
  get_in_content(POP_INSTRUCTION_CODE);
}

void xchg(char* a, char* b){
  if (a[strlen(a) - 1] == '\n') {
        a[strlen(a) - 1] = '\0';
    }
  if (b[strlen(b) - 1] == '\n') {
      b[strlen(b) - 1] = '\0';
  }
  unsigned int hex_value;
  sscanf(b, "%d", &hex_value);
  char r2[8];
  sprintf(r2, "%x", hex_value);
  sscanf(a, "%d", &hex_value);
  char r1[8];
  sprintf(r1, "%x", hex_value);
  char CODE[13]; 
  strcpy(CODE, "0x400");
  strcat(CODE, r1);
  strcat(CODE, r2);
  strcat(CODE, "000");
  int XCHG_INSTRUCTION_CODE;
  sscanf(CODE, "%x", &XCHG_INSTRUCTION_CODE);
  get_in_content(XCHG_INSTRUCTION_CODE);
}

void add(char* a, char* b){
  if (a[strlen(a) - 1] == '\n') {
        a[strlen(a) - 1] = '\0';
    }
  if (b[strlen(b) - 1] == '\n') {
      b[strlen(b) - 1] = '\0';
  }
  unsigned int hex_value;
  sscanf(b, "%d", &hex_value);
  char r2[8];
  sprintf(r2, "%x", hex_value);
  sscanf(a, "%d", &hex_value);
  char r1[8];
  sprintf(r1, "%x", hex_value);
  char CODE[13];
  strcpy(CODE, "0x50");
  strcat(CODE, r2);
  strcat(CODE, r2);
  strcat(CODE, r1);
  strcat(CODE, "000");
  int ADD_INSTRUCTION_CODE;
  sscanf(CODE, "%x", &ADD_INSTRUCTION_CODE);
  get_in_content(ADD_INSTRUCTION_CODE);
}

void sub(char* a, char* b){
  if (a[strlen(a) - 1] == '\n') {
        a[strlen(a) - 1] = '\0';
    }
  if (b[strlen(b) - 1] == '\n') {
      b[strlen(b) - 1] = '\0';
  }
  unsigned int hex_value;
  sscanf(b, "%d", &hex_value);
  char r2[8];
  sprintf(r2, "%x", hex_value);
  sscanf(a, "%d", &hex_value);
  char r1[8];
  sprintf(r1, "%x", hex_value);
  char CODE[13]; 
  strcpy(CODE, "0x51");
  strcat(CODE, r2);
  strcat(CODE, r2);
  strcat(CODE, r1);
  strcat(CODE, "000");
  int SUB_INSTRUCTION_CODE;
  sscanf(CODE, "%x", &SUB_INSTRUCTION_CODE);
  get_in_content(SUB_INSTRUCTION_CODE);
}

void mul(char* a, char* b){
  if (a[strlen(a) - 1] == '\n') {
        a[strlen(a) - 1] = '\0';
    }
  if (b[strlen(b) - 1] == '\n') {
      b[strlen(b) - 1] = '\0';
  }
  unsigned int hex_value;
  sscanf(b, "%d", &hex_value);
  char r2[8];
  sprintf(r2, "%x", hex_value);
  sscanf(a, "%d", &hex_value);
  char r1[8];
  sprintf(r1, "%x", hex_value);
  char CODE[13]; 
  strcpy(CODE, "0x52");
  strcat(CODE, r2);
  strcat(CODE, r2);
  strcat(CODE, r1);
  strcat(CODE, "000");
  int MUL_INSTRUCTION_CODE;
  sscanf(CODE, "%x", &MUL_INSTRUCTION_CODE);
  get_in_content(MUL_INSTRUCTION_CODE);
}

void ddiv(char* a, char* b){
  if (a[strlen(a) - 1] == '\n') {
        a[strlen(a) - 1] = '\0';
    }
  if (b[strlen(b) - 1] == '\n') {
      b[strlen(b) - 1] = '\0';
  }
  unsigned int hex_value;
  sscanf(b, "%d", &hex_value);
  char r2[8];
  sprintf(r2, "%x", hex_value);
  sscanf(a, "%d", &hex_value);
  char r1[8];
  sprintf(r1, "%x", hex_value);
  char CODE[13];
  strcpy(CODE, "0x53");
  strcat(CODE, r2);
  strcat(CODE, r2);
  strcat(CODE, r1);
  strcat(CODE, "000");
  int DIV_INSTRUCTION_CODE;
  sscanf(CODE, "%x", &DIV_INSTRUCTION_CODE);
  get_in_content(DIV_INSTRUCTION_CODE);
}

void not(char* a){
  if (a[strlen(a) - 1] == '\n') {
    a[strlen(a) - 1] = '\0';
  }
  unsigned int hex_value;
  sscanf(a, "%d", &hex_value);
  char r1[8];
  sprintf(r1, "%x", hex_value);
  char CODE[13]; 
  strcpy(CODE, "0x60");
  strcat(CODE, r1);
  strcat(CODE, r1);
  strcat(CODE, "0000");
  int NOT_INSTRUCTION_CODE;
  sscanf(CODE, "%x", &NOT_INSTRUCTION_CODE);
  get_in_content(NOT_INSTRUCTION_CODE);
}
void and(char* a, char* b){
  if (a[strlen(a) - 1] == '\n') {
        a[strlen(a) - 1] = '\0';
    }
  if (b[strlen(b) - 1] == '\n') {
      b[strlen(b) - 1] = '\0';
  }
  unsigned int hex_value;
  sscanf(b, "%d", &hex_value);
  char r2[8];
  sprintf(r2, "%x", hex_value);
  sscanf(a, "%d", &hex_value);
  char r1[8];
  sprintf(r1, "%x", hex_value);
  char CODE[13];
  strcpy(CODE, "0x61");
  strcat(CODE, r2);
  strcat(CODE, r2);
  strcat(CODE, r1);
  strcat(CODE, "000");
  int AND_INSTRUCTION_CODE;
  sscanf(CODE, "%x", &AND_INSTRUCTION_CODE);
  get_in_content(AND_INSTRUCTION_CODE);
}
void or(char* a, char* b){
  if (a[strlen(a) - 1] == '\n') {
        a[strlen(a) - 1] = '\0';
    }
  if (b[strlen(b) - 1] == '\n') {
      b[strlen(b) - 1] = '\0';
  }
  unsigned int hex_value;
  sscanf(b, "%d", &hex_value);
  char r2[8];
  sprintf(r2, "%x", hex_value);
  sscanf(a, "%d", &hex_value);
  char r1[8];
  sprintf(r1, "%x", hex_value);
  char CODE[13];
  strcpy(CODE, "0x62");
  strcat(CODE, r2);
  strcat(CODE, r2);
  strcat(CODE, r1);
  strcat(CODE, "000");
  int OR_INSTRUCTION_CODE;
  sscanf(CODE, "%x", &OR_INSTRUCTION_CODE);
  get_in_content(OR_INSTRUCTION_CODE);
}
void xor(char* a, char* b){
  if (a[strlen(a) - 1] == '\n') {
        a[strlen(a) - 1] = '\0';
    }
  if (b[strlen(b) - 1] == '\n') {
      b[strlen(b) - 1] = '\0';
  }
  unsigned int hex_value;
  sscanf(b, "%d", &hex_value);
  char r2[8];
  sprintf(r2, "%x", hex_value);
  sscanf(a, "%d", &hex_value);
  char r1[8];
  sprintf(r1, "%x", hex_value);
  char CODE[13];
  strcpy(CODE, "0x63");
  strcat(CODE, r2);
  strcat(CODE, r2);
  strcat(CODE, r1);
  strcat(CODE, "000");
  int XOR_INSTRUCTION_CODE;
  sscanf(CODE, "%x", &XOR_INSTRUCTION_CODE);
  get_in_content(XOR_INSTRUCTION_CODE);
}

void shl(char* a, char* b){
  if (a[strlen(a) - 1] == '\n') {
        a[strlen(a) - 1] = '\0';
    }
  if (b[strlen(b) - 1] == '\n') {
      b[strlen(b) - 1] = '\0';
  }
  unsigned int hex_value;
  sscanf(b, "%d", &hex_value);
  char r2[8];
  sprintf(r2, "%x", hex_value);
  sscanf(a, "%d", &hex_value);
  char r1[8];
  sprintf(r1, "%x", hex_value);
  char CODE[13]; 
  strcpy(CODE, "0x70");
  strcat(CODE, r2);
  strcat(CODE, r2);
  strcat(CODE, r1);
  strcat(CODE, "000");
  int SHL_INSTRUCTION_CODE;
  sscanf(CODE, "%x", &SHL_INSTRUCTION_CODE);
  get_in_content(SHL_INSTRUCTION_CODE);
}
void shr(char* a, char* b){
  if (a[strlen(a) - 1] == '\n') {
        a[strlen(a) - 1] = '\0';
    }
  if (b[strlen(b) - 1] == '\n') {
      b[strlen(b) - 1] = '\0';
  }
  unsigned int hex_value;
  sscanf(b, "%d", &hex_value);
  char r2[8];
  sprintf(r2, "%x", hex_value);
  sscanf(a, "%d", &hex_value);
  char r1[8];
  sprintf(r1, "%x", hex_value);
  char CODE[13]; 
  strcpy(CODE, "0x71");
  strcat(CODE, r2);
  strcat(CODE, r2);
  strcat(CODE, r1);
  strcat(CODE, "000");
  int SHR_INSTRUCTION_CODE;
  sscanf(CODE, "%x", &SHR_INSTRUCTION_CODE);
  get_in_content(SHR_INSTRUCTION_CODE);
}

void ld(char*operand, char*reg, char* op){
  if (reg[strlen(reg) - 1] == '\n') {
        reg[strlen(reg) - 1] = '\0';
    }
  if (operand[strlen(operand) - 1] == '\n') {
      operand[strlen(operand) - 1] = '\0';
  }
  if (op[strlen(op) - 1] == '\n') {
      op[strlen(op) - 1] = '\0';
  }
  if(strcmp(reg, "p")==0){
    reg = strdup("e");
  }
  else if(strcmp(reg, "c")==0){
    reg = strdup("f");
  }
  else{
    unsigned int hex_value;
    sscanf(reg, "%d", &hex_value);
    char r2[8];
    sprintf(r2, "%x", hex_value);
    reg = strdup(r2);
  }
  if(strcmp(op,"regmem")==0){
    char* numStart = strchr(operand, 'r');
    if (numStart != NULL) {
      numStart++; 
      
      char numStr[3]; 
      int i = 0;
      while (isdigit(numStart[i])) {
          numStr[i] = numStart[i];
          i++;
      }
      numStr[i] = '\0';  
      char CODE[13]; 
      strcpy(CODE, "0x93");
      strcat(CODE, reg);
      strcat(CODE, numStr);
      strcat(CODE, "0000");
      int LD_INSTRUCTION_CODE;
      sscanf(CODE, "%x", &LD_INSTRUCTION_CODE);
      get_in_content(LD_INSTRUCTION_CODE);
    }
  }
  else if(strcmp(op,"literal")==0){
    unsigned int literal;
    if(strncmp(operand, "0x", 2) == 0 || strncmp(operand, "0X", 2) == 0) sscanf(operand, "%x", &literal);
    else literal = atoi(operand);
    if(literal > 4095){
      current_section->global_literal_pool = add_literal(current_section->global_literal_pool, NULL, literal, current_section->label, current_section->my_location_counter*4+1);
      char CODE[13];
      strcpy(CODE, "0x92");
      strcat(CODE, reg);
      if(strcmp(reg, "e")==0) strcat(CODE, "f0000");
      else strcat(CODE, "f0000");
      int pomeraj_do_bazena = current_section->size + get_literal_id(literal, NULL) *4 - (current_section->my_location_counter);
      int LD_INSTRUCTION_CODE;
      sscanf(CODE, "%x", &LD_INSTRUCTION_CODE);
      LD_INSTRUCTION_CODE+=pomeraj_do_bazena;
      get_in_content(LD_INSTRUCTION_CODE);
    }
    else{
      char CODE[13]; 
      strcpy(CODE, "0x91");
      strcat(CODE, reg);
      strcat(CODE, "00000");
      unsigned int LD_INSTRUCTION_CODE;
      sscanf(CODE, "%x", &LD_INSTRUCTION_CODE);
      LD_INSTRUCTION_CODE+=literal;
      int num_elements = current_section->size;
      get_in_content(LD_INSTRUCTION_CODE);

    }
  }
  else if(strcmp(op,"simbol")==0){
    int defined = 0;
    if(exists_in_sym_table(operand)){
      for(struct symbol_table* current = global_symbol_table; current; current = current->next){
        if(strcmp(current->label, operand)==0){
          defined = current->local;
        }
      }
    }
    if(exists_in_sym_table(operand)){
      if(defined==1){
        char CODE[13]; 
        strcpy(CODE, "0x91");
        strcat(CODE, reg);
        strcat(CODE, "00000");
        current_section->global_literal_pool = add_literal(current_section->global_literal_pool, operand, 0, current_section->label, current_section->my_location_counter*4+1);
        int pomeraj_do_bazena = current_section->size + get_literal_id(-1, operand) *4 - (current_section->my_location_counter);
        int LD_INSTRUCTION_CODE;
        sscanf(CODE, "%x", &LD_INSTRUCTION_CODE);
        LD_INSTRUCTION_CODE+=pomeraj_do_bazena;
        get_in_content(LD_INSTRUCTION_CODE);

      }
      else{
        //postoji u tabeli simbola ali nije definisan znaci globalan
        char CODE[13];
        strcpy(CODE, "0x92");
        strcat(CODE, reg);
        strcat(CODE, "f0000");
        current_section->global_literal_pool = add_literal(current_section->global_literal_pool, operand, 0, current_section->label, current_section->my_location_counter*4+1);
        int pomeraj_do_bazena = current_section->size + get_literal_id(-1, operand) *4 - (current_section->my_location_counter);
        int LD_INSTRUCTION_CODE;
        sscanf(CODE, "%x", &LD_INSTRUCTION_CODE);
        LD_INSTRUCTION_CODE+=pomeraj_do_bazena;
        get_in_content(LD_INSTRUCTION_CODE);
        add_relocation(current_section->label, current_section->size + get_literal_id(-1, operand) *4, "pc", operand, -4);
      }
    }
    else{
      // ne postoji u tabeli simbola
      add_symbol(operand, current_section->label, current_section->my_location_counter*4+1);
      current_section->global_literal_pool = add_literal(current_section->global_literal_pool, operand, 0, current_section->label, current_section->my_location_counter*4+1);
      add_relocation(current_section->label, current_section->size + get_literal_id(-1, operand) *4, "pc", operand, -4);
      char CODE[13]; 
      strcpy(CODE, "0x92");
      strcat(CODE, reg);
      strcat(CODE, "f0000");
      int LD_INSTRUCTION_CODE;
      sscanf(CODE, "%x", &LD_INSTRUCTION_CODE);
      int pomeraj_do_bazena = current_section->size + get_literal_id(-1, operand) *4 - (current_section->my_location_counter);
      LD_INSTRUCTION_CODE+=pomeraj_do_bazena;
      get_in_content(LD_INSTRUCTION_CODE);
    }
  }
  else if(strcmp(op,"reg")==0){
    char CODE[13]; 
    strcpy(CODE, "0x91");
    strcat(CODE, reg);
    strcat(CODE, operand);
    strcat(CODE, "0000");
    int LD_INSTRUCTION_CODE;
    sscanf(CODE, "%x", &LD_INSTRUCTION_CODE);
    get_in_content(LD_INSTRUCTION_CODE);
  }
  else if(strcmp(op,"literalmem")==0){
    current_section->global_literal_pool = add_literal(current_section->global_literal_pool, NULL, atoi(operand), current_section->label, current_section->my_location_counter*4+1);
    char CODE1[13];
    strcpy(CODE1, "0x92f");
    strcat(CODE1, "00000");
    int pomeraj_do_bazena = current_section->size + get_literal_id(atoi(operand), NULL) *4 - (current_section->my_location_counter);
    int LD_INSTRUCTION_CODE1;
    sscanf(CODE1, "%x", &LD_INSTRUCTION_CODE1);
    LD_INSTRUCTION_CODE1+=pomeraj_do_bazena;
    get_in_content(LD_INSTRUCTION_CODE1);
    char CODE2[13];
    strcpy(CODE2, "0x92");
    strcat(CODE2, reg);
    strcat(CODE2, reg);
    strcat(CODE2, "0000");
    int LD_INSTRUCTION_CODE2 = atoi(CODE2);
    sscanf(CODE2, "%x", &LD_INSTRUCTION_CODE2);
    get_in_content(LD_INSTRUCTION_CODE2);
  }
  else if(strcmp(op, "simbolmem")==0){
    int i=0;
    char real_op[30];
    while(operand[i]!=','){
      real_op[i] = operand[i];
      i++;
    }
    real_op[i]='\0';
    operand = strdup(real_op);
    int defined = 0;
    if(exists_in_sym_table(operand)){
      for(struct symbol_table* current = global_symbol_table; current; current = current->next){
        if(strcmp(current->label, operand)==0){
          defined = current->local;
        }
      }
    }
    if(exists_in_sym_table(operand)){
      if(defined==1){
        char CODE1[13];
        strcpy(CODE1, "0x92");
        strcat(CODE1, reg);
        strcat(CODE1, "f0000");
        int pomeraj_do_bazena = current_section->size + get_literal_id(-1, operand) *4 - (current_section->my_location_counter);
        int LD_INSTRUCTION_CODE1;
        sscanf(CODE1, "%x", &LD_INSTRUCTION_CODE1);
        LD_INSTRUCTION_CODE1+=pomeraj_do_bazena;
        get_in_content(LD_INSTRUCTION_CODE1);
        char CODE2[13];
        strcpy(CODE2, "0x92");
        strcat(CODE2, reg);
        strcat(CODE2, reg);
        strcat(CODE2, "0000");
        int LD_INSTRUCTION_CODE2;
        sscanf(CODE2, "%x", &LD_INSTRUCTION_CODE2);
        get_in_content(LD_INSTRUCTION_CODE2);
      }
      else{
        current_section->global_literal_pool = add_literal(current_section->global_literal_pool, operand, 0, current_section->label, current_section->my_location_counter*4+1);
        char CODE1[13]; 
        strcpy(CODE1, "0x92");
        strcat(CODE1, reg);
        strcat(CODE1, "f0000");
        int pomeraj_do_bazena = current_section->size + get_literal_id(-1, operand) *4 - (current_section->my_location_counter);
        int LD_INSTRUCTION_CODE1;
        sscanf(CODE1, "%x", &LD_INSTRUCTION_CODE1);
        LD_INSTRUCTION_CODE1+=pomeraj_do_bazena;
        get_in_content(LD_INSTRUCTION_CODE1);
        char CODE2[13]; 
        strcpy(CODE2, "0x92");
        strcat(CODE2, reg);
        strcat(CODE2, reg);
        strcat(CODE2, "0000");
        int LD_INSTRUCTION_CODE2;
        sscanf(CODE2, "%x", &LD_INSTRUCTION_CODE2);
        get_in_content(LD_INSTRUCTION_CODE2);
        add_relocation(current_section->label, current_section->size + get_literal_id(-1, operand) *4, "pc", operand, -4);
      }
    }
    else{
      add_symbol(operand, current_section->label, current_section->my_location_counter*1);
      current_section->global_literal_pool = add_literal(current_section->global_literal_pool, operand, 0, current_section->label, current_section->my_location_counter*4+1);
      char CODE1[13]; 
      strcpy(CODE1, "0x92");
      strcat(CODE1, reg);
      strcat(CODE1, "f0000");
      int pomeraj_do_bazena = current_section->size + get_literal_id(-1, operand) *4 - (current_section->my_location_counter);
      int LD_INSTRUCTION_CODE1;
      sscanf(CODE1, "%x", &LD_INSTRUCTION_CODE1);
      LD_INSTRUCTION_CODE1+=pomeraj_do_bazena;
      add_relocation(current_section->label, current_section->size + get_literal_id(-1, operand) *4, "pc", operand, -4);
      get_in_content(LD_INSTRUCTION_CODE1);
      char CODE2[13]; 
      strcpy(CODE2, "0x92");
      strcat(CODE2, reg);
      strcat(CODE2, reg);
      strcat(CODE2, "0000");
      int LD_INSTRUCTION_CODE2;
      sscanf(CODE2, "%x", &LD_INSTRUCTION_CODE2);
      get_in_content(LD_INSTRUCTION_CODE2);
    }
  }
  else if(strcmp(op, "reglit")==0){
    char reg_value[256];
    char offset_value[256];

    const char *open_bracket = strstr(operand, "[");
    const char *plus_sign = strstr(operand, "+");

    if (open_bracket != NULL && plus_sign != NULL) {
      strncpy(reg_value, open_bracket + 2, plus_sign - open_bracket - 3);
      reg_value[plus_sign - open_bracket - 3] = '\0';
      strncpy(offset_value, plus_sign + 2, strcspn(plus_sign + 2, "]\n"));
      offset_value[strcspn(plus_sign + 2, "]\n")] = '\0';
    }
    char* registar;
    if(strcmp(reg_value, "sp")==0){
      registar = strdup("e");
    }
    else if(strcmp(reg_value, "pc")==0){
      registar = strdup("f");
    }
    else {
      unsigned int hex_value;
      sscanf(reg_value+1, "%d", &hex_value);
      char r2[8];
      sprintf(r2, "%x", hex_value);
      registar = strdup(r2);
    }

    unsigned int literal;
    if(strncmp(offset_value, "0x", 2) == 0 || strncmp(offset_value, "0X", 2) == 0) {
      sscanf(offset_value, "%x", &literal);
    }
    else literal = atoi(offset_value);
    if(literal > 4095){
      printf("Greska pri asembliranju\n");
      exit(-1);
    }
    else{
      char CODE[13];
      strcpy(CODE, "0x92");
      strcat(CODE, reg);
      strcat(CODE, registar);
      strcat(CODE, "0000");
      unsigned int LD_INSTRUCTION_CODE;
      sscanf(CODE, "%x", &LD_INSTRUCTION_CODE);
      LD_INSTRUCTION_CODE+=literal;
      get_in_content(LD_INSTRUCTION_CODE);
    }
  }
  else if(strcmp(op, "regsim")==0){
    char reg_value[256];
    char simbol[256];

    const char *open_bracket = strstr(operand, "[");
    const char *plus_sign = strstr(operand, "+");

    if (open_bracket != NULL && plus_sign != NULL) {
      strncpy(reg_value, open_bracket + 2, plus_sign - open_bracket - 3);
      reg_value[plus_sign - open_bracket - 3] = '\0';
      strncpy(simbol, plus_sign + 2, strcspn(plus_sign + 2, "]\n"));
      simbol[strcspn(plus_sign + 2, "]\n")] = '\0';
    }
    char* registar;
    if(strcmp(reg_value, "sp")==0){
      registar = strdup("e");
    }
    else if(strcmp(reg_value, "pc")==0){
      registar = strdup("f");
    }
    else {
      unsigned int hex_value;
      sscanf(reg_value+1, "%d", &hex_value);
      char r2[8];
      sprintf(r2, "%x", hex_value);
      registar = strdup(r2);
    }
    int defined = 0;
    if(exists_in_sym_table(operand)){
      for(struct symbol_table* current = global_symbol_table; current; current = current->next){
        if(strcmp(current->label, operand)==0){
          defined = current->local;
        }
      }
    }
    if(exists_in_sym_table(simbol) && defined==1){
      int off;
      for(struct symbol_table* current = global_symbol_table; current; current = current->next){
        if(strcmp(current->label, operand)==0){
          off = current->offset;
        }
      }
      if(off<4096){
        char CODE[13];
        strcpy(CODE, "0x92");
        strcat(CODE, reg);
        strcat(CODE, registar);
        strcat(CODE, "0000");
        unsigned int LD_INSTRUCTION_CODE;
        sscanf(CODE, "%x", &LD_INSTRUCTION_CODE);
        LD_INSTRUCTION_CODE+=off;
        get_in_content(LD_INSTRUCTION_CODE);
      }
      else{
        printf("Greska pri asembliranju\n");
        exit(-1);
      }
    }
    else{
      printf("Greska pri asembliranju\n");
      exit(-1);
    }
  }
}

void st(char*operand, char*reg, char* op){
  if (reg[strlen(reg) - 1] == '\n') {
        reg[strlen(reg) - 1] = '\0';
    }
  if (operand[strlen(operand) - 1] == '\n') {
      operand[strlen(operand) - 1] = '\0';
  }
  if (op[strlen(op) - 1] == '\n') {
      op[strlen(op) - 1] = '\0';
  }
  char* regStart = reg;
  char* areg;
  if (regStart != NULL) {
    char regStr[3];  
    int i = 0;
    while (isdigit(regStart[i])) {
        regStr[i] = regStart[i];
        i++;
    }
    regStr[i] = '\0';
    areg = strdup(regStr);
  }
  if(strcmp(op,"regmem")==0){
    char* numStart = strchr(operand, 'r');
    if (numStart != NULL) {
      numStart++;
      
      char numStr[3];  
      int i = 0;
      while (isdigit(numStart[i])) {
          numStr[i] = numStart[i];
          i++;
      }
      numStr[i] = '\0';  
      char CODE[13]; 
      strcpy(CODE, "0x80");
      strcat(CODE, numStr);
      strcat(CODE, "0");
      strcat(CODE, areg);
      strcat(CODE, "000");
      int ST_INSTRUCTION_CODE;
      sscanf(CODE, "%x", &ST_INSTRUCTION_CODE);
      get_in_content(ST_INSTRUCTION_CODE);
    }
  }
  else if(strcmp(op,"literal")==0){
    printf("Greska u adresiranju!\n");
    exit(-1);
  }
  else if(strcmp(op,"simbol")==0){
    printf("Greska pri adresiranju. \n");
    exit(-1);
  }
  else if(strcmp(op,"reg")==0){
    char CODE[13]; 
    strcpy(CODE, "0x91");
    strcat(CODE, areg);
    strcat(CODE, operand);
    strcat(CODE, "0000");
    int ST_INSTRUCTION_CODE;
    sscanf(CODE, "%x", &ST_INSTRUCTION_CODE);
    get_in_content(ST_INSTRUCTION_CODE);
  }
  else if(strcmp(op,"literalmem")==0){
    current_section->global_literal_pool = add_literal(current_section->global_literal_pool, NULL, atoi(operand), current_section->label, current_section->my_location_counter*4+1);
    char CODE1[13]; 
    strcpy(CODE1, "0x92f");
    strcat(CODE1, "00000");
    int pomeraj_do_bazena = current_section->size + get_literal_id(atoi(operand), NULL) *4 - (current_section->my_location_counter);
    int ST_INSTRUCTION_CODE1;
    sscanf(CODE1, "%x", &ST_INSTRUCTION_CODE1);
    ST_INSTRUCTION_CODE1+=pomeraj_do_bazena;
    get_in_content(ST_INSTRUCTION_CODE1);
    char CODE2[13];
    strcpy(CODE2, "0x92");
    strcat(CODE2, areg);
    strcat(CODE2, areg);
    strcat(CODE2, "0000");
    int ST_INSTRUCTION_CODE2;
    sscanf(CODE2, "%x", &ST_INSTRUCTION_CODE2);
    get_in_content(ST_INSTRUCTION_CODE2);
  }
  else if(strcmp(op,"simbolmem")==0){
    int defined = 0;
    if(exists_in_sym_table(operand)){
      for(struct symbol_table* current = global_symbol_table; current; current = current->next){
        if(strcmp(current->label, operand)==0){
          defined = current->local;
        }
      }
    }
    if(exists_in_sym_table(operand) && defined==1){
      char CODE1[13];
      strcpy(CODE1, "0x82f0");
      strcat(CODE1, areg);
      // int off;
      // for(struct symbol_table* current = global_symbol_table; current; current=current->next){
      //   if(strcmp(current->label, operand)==0){
      //     off = current->offset;
      //   }
      // }
      strcat(CODE1, "000");
      current_section->global_literal_pool = add_literal(current_section->global_literal_pool, operand, 0, current_section->label, current_section->my_location_counter*4+1);
      int pomeraj_do_bazena = current_section->size + get_literal_id(-1, operand) *4 - (current_section->my_location_counter);
      int ST_INSTRUCTION_CODE1;
      sscanf(CODE1, "%x", &ST_INSTRUCTION_CODE1);
      ST_INSTRUCTION_CODE1+=pomeraj_do_bazena;
      get_in_content(ST_INSTRUCTION_CODE1);
    }
    else{
      if(!exists_in_sym_table(operand)) add_symbol(operand, current_section->label, current_section->my_location_counter*1);
      current_section->global_literal_pool = add_literal(current_section->global_literal_pool, operand, 0, current_section->label, current_section->my_location_counter*4+1);
      char CODE1[13]; 
      strcpy(CODE1, "0x82f0");
      strcat(CODE1, areg);
      strcat(CODE1, "000");
      int pomeraj_do_bazena = current_section->size + get_literal_id(-1, operand) *4 - (current_section->my_location_counter);
      int ST_INSTRUCTION_CODE1;
      sscanf(CODE1, "%x", &ST_INSTRUCTION_CODE1);
      ST_INSTRUCTION_CODE1+=pomeraj_do_bazena;
      get_in_content(ST_INSTRUCTION_CODE1);
    }
  }
  else if(strcmp(op, "reglit")==0){
    char reg_value[256];
    char offset_value[256];

    const char *open_bracket = strstr(operand, "[");
    const char *plus_sign = strstr(operand, "+");

    if (open_bracket != NULL && plus_sign != NULL) {
      strncpy(reg_value, open_bracket + 2, plus_sign - open_bracket - 3);
      reg_value[plus_sign - open_bracket - 3] = '\0';
      strncpy(offset_value, plus_sign + 2, strcspn(plus_sign + 2, "]\n"));
      offset_value[strcspn(plus_sign + 2, "]\n")] = '\0';
    }
    char* registar;
    if(strcmp(reg_value, "sp")==0){
      registar = strdup("e");
    }
    else if(strcmp(reg_value, "pc")==0){
      registar = strdup("f");
    }
    else {
      unsigned int hex_value;
      sscanf(reg_value+1, "%d", &hex_value);
      char r2[8];
      sprintf(r2, "%x", hex_value);
      registar = strdup(r2);
    }

    unsigned int literal;
    if(strncmp(offset_value, "0x", 2) == 0 || strncmp(offset_value, "0X", 2) == 0) {
      sscanf(offset_value, "%x", &literal);
    }
    else literal = atoi(offset_value);
    if(literal > 4095){
      printf("Greska pri asembliranju\n");
      exit(-1);
    }
    else{
      char CODE[13];
      strcpy(CODE, "0x80");
      strcat(CODE, registar); //A
      strcat(CODE, "0"); // B
      strcat(CODE, areg); // C
      strcat(CODE, "000"); // D
      unsigned int ST_INSTRUCTION_CODE;
      sscanf(CODE, "%x", &ST_INSTRUCTION_CODE);
      ST_INSTRUCTION_CODE+=literal;
      get_in_content(ST_INSTRUCTION_CODE);
    }
  }
  else if(strcmp(op, "regsim")==0){
    char reg_value[256];
    char simbol[256];

    const char *open_bracket = strstr(operand, "[");
    const char *plus_sign = strstr(operand, "+");

    if (open_bracket != NULL && plus_sign != NULL) {
      strncpy(reg_value, open_bracket + 2, plus_sign - open_bracket - 3);
      reg_value[plus_sign - open_bracket - 3] = '\0';
      strncpy(simbol, plus_sign + 2, strcspn(plus_sign + 2, "]\n"));
      simbol[strcspn(plus_sign + 2, "]\n")] = '\0';
    }
    char* registar;
    if(strcmp(reg_value, "sp")==0){
      registar = strdup("e");
    }
    else if(strcmp(reg_value, "pc")==0){
      registar = strdup("f");
    }
    else {
      unsigned int hex_value;
      sscanf(reg_value+1, "%d", &hex_value);
      char r2[8];
      sprintf(r2, "%x", hex_value);
      registar = strdup(r2);
    }
    int defined = 0;
    if(exists_in_sym_table(operand)){
      for(struct symbol_table* current = global_symbol_table; current; current = current->next){
        if(strcmp(current->label, operand)==0){
          defined = current->local;
        }
      }
    }
    if(exists_in_sym_table(simbol) && defined==1){
      int off;
      for(struct symbol_table* current = global_symbol_table; current; current = current->next){
        if(strcmp(current->label, operand)==0){
          off = current->offset;
        }
      }
      if(off<4096){
        char CODE[13]; 
        strcpy(CODE, "0x80");
        strcat(CODE, registar); //A
        strcat(CODE, "0"); // B
        strcat(CODE, areg); // C
        strcat(CODE, "000"); // D
        unsigned int ST_INSTRUCTION_CODE;
        sscanf(CODE, "%x", &ST_INSTRUCTION_CODE);
        ST_INSTRUCTION_CODE+=off;
        get_in_content(ST_INSTRUCTION_CODE);
      }
      else{
        printf("Greska pri asembliranju\n");
        exit(-1);
      }
    }
    else{
      printf("Greska pri asembliranju\n");
      exit(-1);
    }
  }
}

void instruction_cssrd(char* a, char* b){
  if (a[strlen(a) - 1] == '\n') {
        a[strlen(a) - 1] = '\0';
    }
  if (b[strlen(b) - 1] == '\n') {
      b[strlen(b) - 1] = '\0';
  }
  sscanf(a, "%[^,]", a);
  char* arg = "";
  if(strcmp(a, "cause")==0){
    arg = "2";
  }
  else if(strcmp(a, "status")==0){
    arg = "0";
  }
  else if(strcmp(a, "handler")==0){
    arg = "1";
  }
  char CODE[13];
  strcpy(CODE, "0x90");
  strcat(CODE, b);
  strcat(CODE, arg);
  strcat(CODE, "0000");
  unsigned int CSSRD_INSTRUCTION_CODE;
  sscanf(CODE, "%x", &CSSRD_INSTRUCTION_CODE);
  get_in_content(CSSRD_INSTRUCTION_CODE);
}
void csrwr(char* a, char* b){
  if (a[strlen(a) - 1] == '\n') {
        a[strlen(a) - 1] = '\0';
    }
  if (b[strlen(b) - 1] == '\n') {
      b[strlen(b) - 1] = '\0';
  }
  char* arg = "";
  if(strcmp(b, "cause")==0){
    arg = "2";
  }
  else if(strcmp(b, "status")==0){
    arg = "0";
  }
  else if(strcmp(b, "handler")==0){
    arg = "1";
  }
  int num;
  sscanf(a, "%d,", &num);
  char str[3];
  snprintf(str, sizeof(str), "%d", num);
  char CODE[13]; 
  strcpy(CODE, "0x94");
  strcat(CODE, arg);
  strcat(CODE, str);
  strcat(CODE, "0000");
  int CSRWR_INSTRUCTION_CODE;
  sscanf(CODE, "%x", &CSRWR_INSTRUCTION_CODE);
  get_in_content(CSRWR_INSTRUCTION_CODE);
}


extern bool first_pass;
int assemble(char* file){
  FILE *myfile = fopen(file, "r");
  if (!myfile) {
    printf("Error while opening file.\n");
    return -1;
  }
  yyin = myfile;
  yyparse();
	first_pass = false;
  fclose(myfile);
  myfile = fopen(file, "r");
  if (!myfile) {
    printf("Error while opening file.\n");
    return -1;
  }
  yyin = myfile;
	yyparse();
}

int main(int argc, char* argv[]){
  char *input_file = NULL;
  if (argc < 4) {
    printf("Usage: %s -o <output_file> <input_file>\n", argv[0]);
    return 1;
  }
  for (int i = 1; i < argc; i++) {
    if (strcmp(argv[i], "-o") == 0 && i + 1 < argc) {
      output_file = argv[i + 1];
      i++; 
    }
  }
  input_file = argv[argc - 1];
  // printf("Input file: %s\n", input_file);
  // printf("Output file: %s\n", output_file);

  assemble(input_file);

  printf("Assembler finished.\n");
}


// gcc parser.c lexer.c src/assembler.c -o assembler
// bison -d -t misc/parser.y
// gcc src/emulator.c -o emulator
// gcc src/linker.c -o linker
// chmod +x start.sh
// ./start.sh
// flex misc/lexer.l