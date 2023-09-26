#include "../inc/linker.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>

char *output_file = NULL;

void checkSymbols(){
  //ako je ostao ijedan externi prijavi gresku
  for(struct sym_table* current = global_sym_table; current; current = current->next){
    if(current->local == 3) {
      printf("Definition of symbol %s is not found\n", current->label);
      exit(-1);
    }
  }
}

bool exists_in_symbol_table(char* label){
  for(struct sym_table* current = global_sym_table; current; current = current->next){
    if(strcmp(current->label, label)==0) return true;
  }
  return false;
}

void add_my_symbols(char* file_name, char* label, char* section, int offset, int num, int local){
  struct sym_table* new_symbol = (struct sym_table*)malloc(sizeof(struct sym_table));
  if (new_symbol == NULL) {
    fprintf(stderr, "Memory allocation error for new symbol.\n");
    exit(EXIT_FAILURE);
  }
  new_symbol->file_name = strdup(file_name);
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
  new_symbol->local = local; 
  new_symbol->num = num; 
  new_symbol->next = NULL; 

  if (my_symbol_table == NULL) {
      my_symbol_table = new_symbol;
  } else {
    struct sym_table* current = my_symbol_table;
    while (current->next != NULL) {
      current = current->next;
    }
    current->next = new_symbol;
  }
}

void add_reloc(char* file_name, char* section, int offset, char*type, char* symbol, int addend){
  struct reloc_table* new_reloc = (struct reloc_table*)malloc(sizeof(struct reloc_table));
  if (new_reloc == NULL) {
      fprintf(stderr, "Memory allocation error for new rel.\n");
      exit(EXIT_FAILURE);
  }
  new_reloc->file_name = strdup(file_name);
  new_reloc->section = strdup(section);
  new_reloc->symbol = strdup(symbol);
  new_reloc->offset = offset;
  new_reloc->addend = addend; 
  new_reloc->type = type; 
  new_reloc->next = NULL; 

  if (global_rel_table == NULL) {
    global_rel_table = new_reloc;
  } 
  else {
    struct reloc_table* current = global_rel_table;
    while (current->next != NULL) {
        current = current->next;
    }
    current->next = new_reloc;
  }
}

void add_symb(char* file_name, char* label, char* section, int offset, int num, int type){
  if(type == 1) return;
  if(exists_in_symbol_table(label)){
    // vec postoji u tabeli
    if (type == 2){
      // globalni se dodaje
      int defined = 0;
      for(struct sym_table* current = global_sym_table; current; current = current->next){
        if(strcmp(current->label, label)==0) {
          defined = current->local;
          break;
        }
      }
      if(defined == 2){
        printf("Error: Two global symbols named: %s\n", label);
        exit(-1);
      }
      else if(defined == 3){
        // postojao je externi u tabeli i sad je dodefinisan
        for(struct sym_table* current = global_sym_table; current; current = current->next){
          if(strcmp(current->label, label)==0) {
            current->file_name = strdup(file_name);
            current->local = 2;
            current->offset = offset;
            current->section = strdup(section);
          }
        }
      }
      else{
        printf("Error in table.\n");
        exit(-1);
      }
    }
    if (type == 3){
      //dodaje se externi
      int defined;
      for(struct sym_table* current = global_sym_table; current; current = current->next){
        if(strcmp(current->label, label)==0) {
          defined = current->local;
          break;
        }
      }
      if(defined == 2){
        //sve je okej nista ne treba raditi postojao je globalni
      }
      else if(defined==3){
        //treba nekako zapamtiti da je definisan extern a nije njegov globalni nikako
      }
      else{
        printf("Error in tabel.\n");
        exit(-1);
      }
    }
  }
  else{
    struct sym_table* new_symbol = (struct sym_table*)malloc(sizeof(struct sym_table));
    if (new_symbol == NULL) {
      fprintf(stderr, "Memory allocation error for new symbol.\n");
      exit(EXIT_FAILURE);
    }
    new_symbol->file_name = strdup(file_name);
    new_symbol->label = strdup(label);
    new_symbol->local = type;
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
    new_symbol->num = num; 
    new_symbol->next = NULL; 

    if (global_sym_table == NULL) {
        global_sym_table = new_symbol;
    } else {
      struct sym_table* current = global_sym_table;
      while (current->next != NULL) {
        current = current->next;
      }
      current->next = new_symbol;
    }
  }
}

void add_sect(char* file_name, char* label, int offset, int size, int my_lc) {
  struct sect_table* new_section = (struct sect_table*)malloc(sizeof(struct sect_table));
  if (new_section == NULL) {
    fprintf(stderr, "Memory allocation error for new section.\n");
    exit(EXIT_FAILURE);
  }
  new_section->file_name = strdup(file_name);
  new_section->label = strdup(label);
  if (new_section->label == NULL) {
    fprintf(stderr, "Memory allocation error for section label.\n");
    free(new_section);
    exit(EXIT_FAILURE);
  }

  new_section->offset = offset;
  new_section->size = size;
  new_section->next = NULL;
  new_section->my_location_counter = my_lc;
  new_section->placed = false;

  if (global_sect_table == NULL) {
    global_sect_table = new_section;
  } 
  else {
    struct sect_table* current_sec = global_sect_table;
    struct sect_table* prev_sec = NULL;

    while (current_sec != NULL) {
      if (strcmp(current_sec->label, label) == 0) {
        break;
      }
      prev_sec = current_sec;
      current_sec = current_sec->next;
    }

    if (current_sec == NULL) {
      prev_sec->next = new_section;
    } else {
      struct sect_table* last_sec_with_same_name = current_sec;
      while (last_sec_with_same_name->next != NULL &&
             strcmp(last_sec_with_same_name->next->label, label) == 0) {
        last_sec_with_same_name = last_sec_with_same_name->next;
      }
      new_section->next = last_sec_with_same_name->next;
      last_sec_with_same_name->next = new_section;
    }
  }
}

void add_to_map(int address, unsigned char content){
  struct map* new_mapping = (struct map*)malloc(sizeof(struct map));
  if (new_mapping == NULL) {
      fprintf(stderr, "Memory allocation error for new mapping.\n");
      exit(EXIT_FAILURE);
  }
  new_mapping->address = address;
  new_mapping->content = content;
  new_mapping->next = NULL; 

  if (global_map == NULL) {
    global_map = new_mapping;
  } 
  else {
    struct map* current = global_map;
    while (current->next != NULL) {
        current = current->next;
    }
    current->next = new_mapping;
  }
}

void initialize(char** files, int num){
  for(int f = 0; f < num; f++){
    FILE *binFile = fopen(files[f], "rb");
    if (!binFile) {
      perror("Nemoguće otvoriti fajl za čitanje");
      return;
    }
    int sizesec = 0;
    int sizesym = 0;
    fread(&sizesec, sizeof(int), 1, binFile);
    fread(&sizesym, sizeof(int), 1, binFile);
    //printf("%d %d\n", sizesec, sizesym);

    char readLabel[16];
    int readOffset = 0;
    int readSize = 0;
    int readMyLc = 0;
    while (sizesec>0){
      fread(&readLabel, sizeof(char), 16, binFile);
      //printf("procitao je labelu: %s ", readLabel);
      fread(&readOffset, sizeof(int), 1, binFile);
      //printf(", offset: %d ", readOffset);
      fread(&readMyLc, sizeof(int), 1, binFile);
      //printf(", lc: %d ", readMyLc);
      fread(&readSize, sizeof(int), 1, binFile);
      //printf(", size: %d ", readSize);
      //printf(", content: ");
      sizesec--;
      add_sect(files[f], readLabel, readOffset, readSize, readMyLc);
      for(struct sect_table* current = global_sect_table; current; current=current->next){
        if(strcmp(current->label, readLabel)==0 && strcmp(current->file_name, files[f])==0){
          int length = current->size;
          //printf("%d\n", length);
          current->content = (char*)malloc(length * sizeof(char));
          for(int i =0 ; i< length; i++){
            unsigned char content;
            fread(&content, sizeof(char), 1, binFile);
            current->content[i] = content;
            //printf("%08x ", current->content[i]);
          }
          //free(current->content);
        }
      }
      //printf("\n");
    }
    
    char readSection[16];
    int readLocal;
    int readNum;
    //citanje simbola
    while (sizesym>0){
      fread(&readLabel, sizeof(char), 16, binFile);
      //printf("procitao je labelu: %s", readLabel);
      fread(&readSection, sizeof(char), 16, binFile);
      //printf(", sekciju: %s ", readSection);
      fread(&readOffset, sizeof(int), 1, binFile);
      //printf(", offset: %d ", readOffset);
      fread(&readLocal, sizeof(int), 1, binFile);
      //printf(", lokal: %d \n", readLocal);
      fread(&readNum, sizeof(int), 1, binFile);
      //printf(", num: %d\n", readNum);
      sizesym--;
      add_symb(files[f], readLabel, readSection, readOffset, readNum, readLocal);
    }
    int addend;
    char type[4];
    //citanje relokacija
    while (fread(&readLabel, sizeof(char), 16, binFile)){
      //printf("procitao je labelu: %s ", readLabel);
      fread(&readSection, sizeof(char), 16, binFile);
      //printf(", sekciju: %s ", readSection);
      fread(&readOffset, sizeof(int), 1, binFile);
      //printf(", offset: %s ", readOffset);
      fread(&type, sizeof(char), 4, binFile);
      //printf(", type: %s ", type);
      fread(&addend, sizeof(int), 1, binFile);
      //printf(", num: %d\n", addend);
      add_reloc(files[f], readSection, readOffset, type, readLabel, addend);
    }
    
    fclose(binFile);

  }
  checkSymbols();
  // for(struct sym_table* current = global_sym_table; current; current=current->next){
  //   printf("Symbol name: %s | Section name: %s | Offset: %x | Type: %d | Redni broj: %d \n", 
  //   current->label, current->section, current->offset, current->local, current->num);
  // }

  // for(struct sect_table* current = global_sect_table; current; current=current->next){
  //   printf("File: %s | Section name: %s | LC: %d | Offset: %d | Size: %d | Content: ", 
  //    current->file_name, current->label, current->my_location_counter, current->offset, current->size);
  //   for(int i=0; i<current->size; i++){
  //     printf("%02x ", current->content[i]);
  //   }
  //   printf("\n");
  // }

  // printf("Relokacioni zapisi: \n");
  // for(struct reloc_table* curr = global_rel_table; curr; curr= curr->next){
  //   printf("Reloc zapis: %s %s %d %s %d %s\n", curr->symbol, curr->section, curr->offset, curr->type, 
  //   curr->addend, curr->file_name);
  // }
}

void mapping(int* place, char** section, int num){
  int sectionStart[num];
  int sectionEnd[num];

  for (int i = 0; i < num; i++) {
    for (struct sect_table* current1 = global_sect_table; current1; current1 = current1->next) {
      if (strcmp(current1->label, section[i]) == 0) {
        sectionStart[i] = place[i];
        sectionEnd[i] = place[i] + current1->size - 1;
      }
    }
  }
  for(int i=0; i<num-1; i++){
    for (int j = i + 1; j < num; j++) {
      if (sectionStart[i] <= sectionEnd[j] && sectionEnd[i] >= sectionStart[j]) {
        fprintf(stderr, "Error: Sections %s and %s overlap.\n", section[i], section[j]);
        exit(1);
      }
    }
  }
  for(int j=0; j<num; j++){
    for(struct sect_table* current = global_sect_table; current; current=current->next){
      if(strcmp(current->label, section[j])==0){
        add_my_symbols(current->file_name, current->label, section[j], 
          place[j], current->size, 1);
        current->offset = place[j];
        for(int i=0; i<current->size; i++){
          add_to_map(place[j], current->content[i]);
          place[j]+=0x1;
        }
        current->placed = true;
      }
    }
  }
  //find max place
  unsigned int max = place[0];
  int indeks=0;
  for(int p = 1; p<2; p++){
    if(place[p]>max){
      max=place[p];
      indeks = p;
    }
  }
  int place2 = max;
  for(struct sect_table* current = global_sect_table; current; current=current->next){
    if(current->placed == false){
      add_my_symbols(current->file_name, current->label, current->label, 
        place2, current->size, 1);
      current->offset = place2;
      for(int i=0; i<current->size; i++){
        add_to_map(place2, current->content[i]);
        place2+= 0x1;
      }
    }
  }
  // for(struct map* current = global_map; current; current=current->next){
  //   printf("Address: %08x, Content: %02x\n", current->address, current->content);
  // }
}

void odredjivanje(){
  for(struct sym_table* symbol = global_sym_table; symbol; symbol=symbol->next){
    for(struct sect_table* sec = global_sect_table; sec; sec=sec->next){
      if(strcmp(sec->label, symbol->section)==0 && strcmp(sec->label, symbol->label)!=0
        && strcmp(sec->file_name, symbol->file_name)==0){
        add_my_symbols(symbol->file_name, symbol->label, symbol->section, 
          sec->offset+symbol->offset, symbol->num, symbol->local);
      }
    }
  }
  // for(struct sym_table* current = my_symbol_table; current; current=current->next){
  //   printf("File: %s | Symbol name: %s | Section name: %s | Offset: %x | Type: %d | Redni broj: %d \n", 
  //   current->file_name, current->label, current->section, current->offset, current->local, current->num);
  // }
}

void razresavanje(){
  FILE* hexFile = fopen(output_file, "w");
  if (!hexFile) {
      perror("Nemoguće otvoriti fajl za pisanje");
      return;
  }
  for(struct reloc_table *relokacija = global_rel_table; relokacija; relokacija=relokacija->next){
    char* symbol = relokacija->symbol;
    char* sekcija = relokacija->section;
    int offset = relokacija->offset;
    int section_offset;
    int symbol_value;
    bool found_symbol = false;
    char* file = relokacija->file_name;
    //nadjem vrednost simbola
    for(struct sym_table* sym = my_symbol_table; sym; sym=sym->next){
      if(strcmp(sym->label, symbol)==0){
        symbol_value = sym->offset;
        found_symbol = true;
        //printf("Vrednost simbola je: %x\n", symbol_value);
      }
    }
    //nadjem pocetak sekcije
    for (struct sect_table* section = global_sect_table; section; section=section->next){
      //printf("%s %s\n", section->file_name, file);
      if(strcmp(section->label, sekcija)==0 && strcmp(section->file_name, file)==0){
        section_offset = section->offset;
        //printf("sekcija: %s\n", section->label);
        break;
      }
    }
    //printf("Sekcija offset: %x\n", section_offset);
    
    if(found_symbol){
      offset = offset+ section_offset; // mesto gde treba da se upise
      //printf("simbol: %s sekcija: %s offset_sekcije: %x na mesto: %x value: %x\n", 
        // symbol, sekcija, section_offset, offset, symbol_value);
      //upisivanje
      for(struct map* mapa = global_map; mapa; mapa=mapa->next){
        for(int s=0; s<4; s++){
          if(mapa){
            if(mapa->address == offset+s){
              mapa->content = (symbol_value>>s*8)&0xFF;
            }
          }
        }
      }
    }
  }

  for(struct map* current = global_map; current; current=current->next){
    fprintf(hexFile, "%08x: ", current->address);
    fprintf(hexFile, "%02x", current->content);
    for (int i = 0; i < 7; i++) {
      if(current->next){
        current = current->next;
        fprintf(hexFile, " %02x", current->content);
      }
      else{
        char nula = 00;
        fprintf(hexFile, " %02x", nula);
      }
    }
    fprintf(hexFile, "\n");
  }
  // for(struct sym_table* current = my_symbol_table; current; current=current->next){
  //   printf("Symbol name: %s | Section name: %s | Offset: %x | Type: %d | Redni broj: %d \n", 
  //   current->label, current->section, current->offset, current->local, current->num);
  // }

  // for(struct sect_table* current = global_sect_table; current; current=current->next){
  //   printf("Section name: %s | LC: %d | Offset: %x | Size: %d | Placed %d | Content: ", 
  //     current->label, current->my_location_counter, current->offset, current->size, current->placed);
  //   for(int i=0; i<current->size/4; i++){
  //     printf("%08x ", current->content[i]);
  //   }
  //   printf("\n");
  // }

  fclose(hexFile);
}

int main(int argc, char *argv[]) {
  char *input_files[argc];
  int num_input_files = 0;
  bool has_hex_option = false;
  bool has_place_option = false;
  bool has_o_option = false;
  char *sections[100]; 
  int offsets[100]; 
  int num_sections = 0;

  if (argc < 4) {
    printf("Usage: %s -hex [-place=<section>@<address>]... -o <output_file> <input_file>...\n", argv[0]);
    return 1;
  }
  for (int i = 1; i < argc; i++) { 
    if (strcmp(argv[i], "-o") == 0) {
      i++;
      if (i < argc) {
        output_file = argv[i];
        has_o_option = true;
      }
    } 
    else if (strncmp(argv[i], "-place=", 7) == 0) {
      char *option = argv[i] + 7; 
      char *section = strtok(option, "@");
      char *offset = strtok(NULL, "@");

      if (section != NULL && offset != NULL) {
        sections[num_sections] = section;
        offsets[num_sections] = (int)strtol(offset, NULL, 16);
        num_sections++;
        has_place_option = true;
      }
    }  
    else if (strcmp(argv[i], "-hex") == 0) {
      has_hex_option = true;
    }
    else {
      input_files[num_input_files] = argv[i];
      num_input_files++;
    }
  }
  if(!has_hex_option || !has_o_option || !has_place_option){
    printf("Usage: %s -hex [-place=<section>@<address>]... -o <output_file> <input_file>...\n", argv[0]);
    return 1; 
  }
  //printf("Izlazna datoteka: %s\n", output_file);

  initialize(input_files, num_input_files);
  mapping(offsets, sections, num_sections);
  odredjivanje();
  razresavanje();
  printf("Linker finished.\n");
  return 0;
}