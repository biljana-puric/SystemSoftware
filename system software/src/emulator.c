#include "../inc/emulator.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <ctype.h>

void emulate(char* file){
  FILE* hexFile = fopen(file, "r");
    if (!hexFile) {
      perror("Nemoguće otvoriti .hex fajl za čitanje");
      return;
  }
  size_t mem_size = 4ULL * 1024 * 1024 * 1024;
  int fd = open("/dev/zero", O_RDWR); 
  void* memory = mmap(NULL, mem_size, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);
  if (memory == MAP_FAILED) {
      perror("mmap");
      return;
  }
  int number_of_instructions=0;
  char line[100];
  while (fgets(line, sizeof(line), hexFile)) {
    unsigned int address;
    sscanf(line, "%x:", &address);
    char* data = strchr(line, ':') + 1;
    int content1, content2;
    char hexString[strlen(data) + 1];
    int j = 0;
    for (int i = 0; data[i]; i++) {
      if (!isspace(data[i])) {
        hexString[j++] = data[i];
      }
    }
    hexString[j] = '\0';
    if (strlen(hexString) >= 8) {
      char firstHex[9];
      char secondHex[9];
      strncpy(firstHex, hexString + 6, 2); 
      strncpy(firstHex + 2, hexString + 4, 2); 
      strncpy(firstHex + 4, hexString + 2, 2); 
      strncpy(firstHex + 6, hexString, 2); 
      firstHex[8] = '\0';
      strncpy(secondHex, hexString + 14, 2);
      strncpy(secondHex + 2, hexString + 12, 2); 
      strncpy(secondHex + 4, hexString + 10, 2); 
      strncpy(secondHex + 6, hexString + 8, 2); 
      secondHex[8] = '\0';
      sscanf(firstHex, "%x", &content1);
      sscanf(secondHex, "%x", &content2);
    } 
    else {
      fprintf(stderr, "Error.\n");
    }
    
    unsigned char* target = memory + address;
    memcpy(target, &content1, sizeof(content1));
    number_of_instructions++;
    target = target + 0x4;
    memcpy(target, &content2, sizeof(content2));
    number_of_instructions++;
    
  }
  fclose(hexFile);
  int broj_registara = 16;
  int velicina_registra_u_bajtima = 4;
  unsigned int* registri = (unsigned int*)malloc(broj_registara * velicina_registra_u_bajtima);
  if (registri == NULL) {
      perror("Nemoguće alocirati memoriju za registre");
      return;
  }
  for(int i = 0; i<broj_registara; i++){
    registri[i] = 0;
  }
  registri[15] = 0x40000000 - 0x4;
  registri[14] = 0x10000000;
  unsigned char* target = memory + registri[15];
  int i=0;
  int num;
  while (i<=number_of_instructions) {
    registri[15] += 0x4; 
    target = memory + registri[15];
    memcpy(&num, target, sizeof(num));
    i++;
    int code  = (num >> 24) & 0xFF;
    switch (code) {
    case 0x00:{
      registri[15] = registri[15] + 0x4;
      printf("Emulated processor executed halt instruction\n");
      printf("Emulated processor state: \n");
      for(int i=0; i<broj_registara; i++){
        printf("r%d = 0x%x\n", i, registri[i]);
      }
      return;
      break;
    }
    case 0x10: {
      //push status; push pc; cause<=4; status<=status&(~0x1); pc<=handle;
      if(num!=0x10000000){ printf("error\n"); return; }
      registri[14] -= 0x4;
      //printf("sp : %x upisuje se : %x\n", registri[14], registri[15]);
      target = memory + registri[14];
      memcpy(target, &registri[15], sizeof(registri[15]));
      registri[14] -= 0x4;
      //printf("sp : %x upisuje se : %x\n", registri[14], registri[0]);
      target = memory + registri[14];
      memcpy(target, &registri[0], sizeof(registri[0]));
      registri[2] = 0x4;
      registri[0] = registri[0] &(~0x1);
      registri[15] = registri[1] - 0x4;
      break;
    }
    case 0x20:{ 
      //push pc; pc<=gpr[A]+gpr[B]+D;
      registri[14] -= 4;
      target = memory + registri[14];
      memcpy(target, &registri[15], sizeof(registri[15]));
      
      unsigned int a = (num >> 20) & 0xF; // e
      unsigned int b = (num >> 16) & 0xF; // f
      unsigned int c = (num >> 12) & 0xF; // 0
      unsigned int d = num & 0xFFF;
      registri[15] = registri[a] + registri[b] + d - 0x4; 
      break;
    }
    case 0x21:{ 
      // push pc; pc<=mem32[gpr[A]+gpr[B]+D];
      registri[14] -= 4;
      target = memory + registri[14];
      memcpy(target, &registri[15], sizeof(registri[15]));
      //printf("pri upisu sp je %x upisuje se %x\n", registri[14], registri[15]);
      unsigned int a = (num >> 20) & 0xF; // e
      unsigned int b = (num >> 16) & 0xF; // f
      unsigned int c = (num >> 12) & 0xF; // 0
      unsigned int d = num & 0xFFF;
      //printf("pc: %x reg a %x reg b %x\n", registri[15], registri[a], registri[b]);
      //registri[15] = registri[a] + registri[b] + d; 
      //printf("pc: %x\n", registri[15]);
      unsigned int pomeraj = registri[a] + registri[b] + d;
      target = memory + pomeraj;
      //printf("pomeraj: %x\n", pomeraj);
      memcpy(&registri[15], target, sizeof(registri[15]));
      //printf("reg: %x\n", registri[15]);
      registri[15] -= 0x4;
      break;
    }
    case 0x30:{ 
      // pc<=gpr[A]+D;
      unsigned int a = (num >> 20) & 0xF; // e
      unsigned int b = (num >> 16) & 0xF; // f
      unsigned int c = (num >> 12) & 0xF; // 0
      int d = num & 0xFFF;
      if (d & 0x800) {
        d = 0x1000 - d;
        registri[15] = registri[a] - d; // Subtract d
      } else {
          registri[15] = registri[a] + d; // Add d
      }
      registri[15] = registri[15] -0x4; // + 8;
      break;
    }
    case 0x31:{ 
      // if (gpr[B] == gpr[C]) pc<=gpr[A]+D;
      unsigned int a = (num >> 20) & 0xF; // e
      unsigned int b = (num >> 16) & 0xF; // f
      unsigned int c = (num >> 12) & 0xF; // 0
      unsigned int d = num & 0xFFF;
      //printf("%x %x %x %x\n", registri[a], registri[b], registri[c], d);
      if(registri[b] == registri[c]) registri[15] = registri[a] + d - 0x4;
      break;
    }
    case 0x32:{ 
      // if (gpr[B] != gpr[C]) pc<=gpr[A]+D;
      unsigned int a = (num >> 20) & 0xF; // e
      unsigned int b = (num >> 16) & 0xF; // f
      unsigned int c = (num >> 12) & 0xF; // 0
      unsigned int d = num & 0xFFF;
      if(registri[b] != registri[c]) registri[15] = registri[a] + d - 0x4;
      break;
    }
    case 0x33:{ 
      // if (gpr[B] signed> gpr[C]) pc<=gpr[A]+D;
      unsigned int a = (num >> 20) & 0xF; // e
      unsigned int b = (num >> 16) & 0xF; // f
      unsigned int c = (num >> 12) & 0xF; // 0
      unsigned int d = num & 0xFFF;
      if(registri[b] > registri[c]) registri[15] = registri[a] + d - 0x4;
      break;
    }
    case 0x38:{ 
      // pc<=mem[gpr[A]+D];
      unsigned int a = (num >> 20) & 0xF; // e
      unsigned int b = (num >> 16) & 0xF; // f
      unsigned int c = (num >> 12) & 0xF; // 0
      unsigned int d = num & 0xFFF;
      unsigned int pomeraj = registri[a] + registri[b] + d; 
      target = memory + pomeraj;
      memcpy(&registri[15], target, sizeof(registri[15]));
      registri[15] = registri[a] + d - 0x4;
      break;
    }
    case 0x39:{ 
      //if (gpr[B] == gpr[C]) pc<=mem32[gpr[A]+D];
      unsigned int a = (num >> 20) & 0xF; // e
      unsigned int b = (num >> 16) & 0xF; // f
      unsigned int c = (num >> 12) & 0xF; // 0
      unsigned int d = num & 0xFFF;
      if(registri[b] == registri[c]){
        unsigned int pomeraj = registri[a] + d; 
        target = memory + pomeraj;
        memcpy(&registri[15], target, sizeof(registri[15]));
        registri[15] = registri[a] + d - 0x4;
      }
      break;
    }
    case 0x3a:{ 
      //if (gpr[B] != gpr[C]) pc<=mem32[gpr[A]+D];
      unsigned int a = (num >> 20) & 0xF; // e
      unsigned int b = (num >> 16) & 0xF; // f
      unsigned int c = (num >> 12) & 0xF; // 0
      unsigned int d = num & 0xFFF;
      if(registri[b] != registri[c]){
        unsigned int pomeraj = registri[a] + d; 
        target = memory + pomeraj;
        memcpy(&registri[15], target, sizeof(registri[15]));
        registri[15] = registri[a] + d - 0x4;
      }
      break;
    }
    case 0x3b:{ 
      //if (gpr[B] > gpr[C]) pc<=mem32[gpr[A]+D];
      unsigned int a = (num >> 20) & 0xF; // e
      unsigned int b = (num >> 16) & 0xF; // f
      unsigned int c = (num >> 12) & 0xF; // 0
      unsigned int d = num & 0xFFF;
      if(registri[b] > registri[c]){
        unsigned int pomeraj = registri[a] + d; 
        target = memory + pomeraj;
        memcpy(&registri[15], target, sizeof(registri[15]));
        registri[15] = registri[a] + d - 0x4;
      }
      break;
    }
    case 0x40:{ 
      //temp<=gpr[B]; gpr[B]<=gpr[C]; gpr[C]<=temp;
      unsigned int a = (num >> 20) & 0xF; // e
      unsigned int b = (num >> 16) & 0xF; // f
      unsigned int c = (num >> 12) & 0xF; // 0
      unsigned int d = num & 0xFFF;
      int temp = registri[b];
      registri[b] = registri[c];
      registri[c] = temp;
      break;
    }
    case 0x50:{ 
      //gpr[A]<=gpr[B] + gpr[C];
      unsigned int a = (num >> 20) & 0xF; // e
      unsigned int b = (num >> 16) & 0xF; // f
      unsigned int c = (num >> 12) & 0xF; // 0
      unsigned int d = num & 0xFFF;
      registri[a] = registri[b] + registri[c];
      break;
    }
    case 0x51:{ 
      //gpr[A]<=gpr[B] - gpr[C];
      unsigned int a = (num >> 20) & 0xF; // e
      unsigned int b = (num >> 16) & 0xF; // f
      unsigned int c = (num >> 12) & 0xF; // 0
      unsigned int d = num & 0xFFF;
      registri[a] = registri[b] - registri[c];
      break;
    }
    case 0x52:{ 
      //gpr[A]<=gpr[B] * gpr[C];
      unsigned int a = (num >> 20) & 0xF; // e
      unsigned int b = (num >> 16) & 0xF; // f
      unsigned int c = (num >> 12) & 0xF; // 0
      unsigned int d = num & 0xFFF;
      registri[a] = registri[b] * registri[c];
      break;
    }
    case 0x53:{ 
      //gpr[A]<=gpr[B] / gpr[C];
      unsigned int a = (num >> 20) & 0xF; // e
      unsigned int b = (num >> 16) & 0xF; // f
      unsigned int c = (num >> 12) & 0xF; // 0
      unsigned int d = num & 0xFFF;
      registri[a] = registri[b] / registri[c];
      break;
    }
    case 0x60:{ 
      //gpr[A]<=~gpr[B];
      unsigned int a = (num >> 20) & 0xF; // e
      unsigned int b = (num >> 16) & 0xF; // f
      unsigned int c = (num >> 12) & 0xF; // 0
      unsigned int d = num & 0xFFF;
      registri[a] = ~registri[b];
      break;
    }
    case 0x61:{ 
      //gpr[A]<=gpr[B] & gpr[C];
      unsigned int a = (num >> 20) & 0xF; // e
      unsigned int b = (num >> 16) & 0xF; // f
      unsigned int c = (num >> 12) & 0xF; // 0
      unsigned int d = num & 0xFFF;
      registri[a] = registri[b] & registri[c];
      break;
    }
    case 0x62:{ 
      //gpr[A]<=gpr[B] | gpr[C];
      unsigned int a = (num >> 20) & 0xF; // e
      unsigned int b = (num >> 16) & 0xF; // f
      unsigned int c = (num >> 12) & 0xF; // 0
      unsigned int d = num & 0xFFF;
      registri[a] = registri[b] | registri[c];
      break;
    }
    case 0x63:{ 
      //gpr[A]<=gpr[B] ^ gpr[C];
      unsigned int a = (num >> 20) & 0xF; // e
      unsigned int b = (num >> 16) & 0xF; // f
      unsigned int c = (num >> 12) & 0xF; // 0
      unsigned int d = num & 0xFFF;
      registri[a] = registri[b] ^ registri[c];
      break;
    }
    case 0x70:{ 
      //gpr[A]<=gpr[B] << gpr[C];
      unsigned int a = (num >> 20) & 0xF; // e
      unsigned int b = (num >> 16) & 0xF; // f
      unsigned int c = (num >> 12) & 0xF; // 0
      unsigned int d = num & 0xFFF;
      registri[a] = registri[b] << registri[c];
      break;
    }
    case 0x71:{ 
      //gpr[A]<=gpr[B] >> gpr[C];
      unsigned int a = (num >> 20) & 0xF; // e
      unsigned int b = (num >> 16) & 0xF; // f
      unsigned int c = (num >> 12) & 0xF; // 0
      unsigned int d = num & 0xFFF;
      registri[a] = registri[b] >> registri[c];
      break;
    }
    case 0x80:{ 
      //mem32[gpr[A]+gpr[B]+D]<=gpr[C];
      unsigned int a = (num >> 20) & 0xF; // e
      unsigned int b = (num >> 16) & 0xF; // f
      unsigned int c = (num >> 12) & 0xF; // 0
      unsigned int d = num & 0xFFF;
      unsigned int pomeraj = registri[a] + registri[b] + d; 
      target = memory + pomeraj;
      memcpy(target, &registri[c], sizeof(registri[c]));
      break;
    }
    case 0x82:{ 
      //mem32[mem32[gpr[A]+gpr[B]+D]]<=gpr[C];
      unsigned int a = (num >> 20) & 0xF; // e
      unsigned int b = (num >> 16) & 0xF; // f
      unsigned int c = (num >> 12) & 0xF; // 0
      unsigned int d = num & 0xFFF;
      unsigned int pomeraj = registri[a] + registri[b] + d; 
      target = memory + pomeraj;
      unsigned int podatak;
      memcpy(&podatak, target, sizeof(podatak));
      target = memory + podatak;
      memcpy(target, &registri[c], sizeof(podatak));
      break;
    }
    case 0x81:{ 
      //gpr[A]<=gpr[A]+D; mem32[gpr[A]]<=gpr[C];
      unsigned int a = (num >> 20) & 0xF; // e
      unsigned int b = (num >> 16) & 0xF; // f
      unsigned int c = (num >> 12) & 0xF; // 0
      int d = num & 0xFFF;
      if (d & 0x800) {
        d = 0x1000 - d;
        registri[a] = registri[a] - d; // Subtract d
      } else {
          registri[a] = registri[a] + d; // Add d
      }
      //printf("pri upisu sp je: %x stavlja se %x\n", registri[a], registri[c]);
      target = memory + registri[a];
      memcpy(target, &registri[c], sizeof(registri[c]));
      break;
    }
    case 0x90:{ 
      //gpr[A]<=csr[B];
      unsigned int a = (num >> 20) & 0xF; // e
      unsigned int b = (num >> 16) & 0xF; // f
      unsigned int c = (num >> 12) & 0xF; // 0
      unsigned int d = num & 0xFFF;
      registri[a] = registri[b];
      break;
    }
    case 0x91:{ 
      //gpr[A]<=gpr[B]+D;
      unsigned int a = (num >> 20) & 0xF; // e
      unsigned int b = (num >> 16) & 0xF; // f
      unsigned int c = (num >> 12) & 0xF; // 0
      unsigned int d = num & 0xFFF;
      registri[a] = registri[b] + d;
      break;
    }
    case 0x92:{ 
      //gpr[A]<=mem32[gpr[B]+gpr[C]+D];
      unsigned int a = (num >> 20) & 0xF; // e
      unsigned int b = (num >> 16) & 0xF; // f
      unsigned int c = (num >> 12) & 0xF; // 0
      unsigned int d = num & 0xFFF;
      unsigned int pomeraj = registri[b] + registri[c] + d;
      target = memory + pomeraj;
      memcpy(&registri[a], target, sizeof(registri[a]));
      //printf("regis: %x\n", registri[a]);
      break;
    }
    case 0x93:{ 
      //gpr[A]<=mem32[gpr[B]]; gpr[B]<=gpr[B]+D;
      unsigned int a = (num >> 20) & 0xF; // e
      unsigned int b = (num >> 16) & 0xF; // f
      unsigned int c = (num >> 12) & 0xF; // 0
      unsigned int d = num & 0xFFF;
      target = memory + registri[b];
      memcpy(&registri[a], target, sizeof(registri[a]));
      //printf(" pri dohvatanju %x sp je: %x\n", registri[a], registri[b]);
      registri[b] = registri[b] + d;
      break;
    }
    case 0x94:{ 
      //csr[A]<=gpr[B];
      unsigned int a = (num >> 20) & 0xF; // e
      unsigned int b = (num >> 16) & 0xF; // f
      unsigned int c = (num >> 12) & 0xF; // 0
      unsigned int d = num & 0xFFF;
      registri[a] = registri[b];
      break;
    }
    case 0x95:{ 
      //csr[A]<=gpr[B] | D;
      unsigned int a = (num >> 20) & 0xF; // e
      unsigned int b = (num >> 16) & 0xF; // f
      unsigned int c = (num >> 12) & 0xF; // 0
      unsigned int d = num & 0xFFF;
      registri[a] = registri[b] | d;
      break;
    }
    case 0x96:{ 
      //csr[A]<=mem32[gpr[B]+gpr[C]+D];
      unsigned int a = (num >> 20) & 0xF; // e
      unsigned int b = (num >> 16) & 0xF; // f
      unsigned int c = (num >> 12) & 0xF; // 0
      unsigned int d = num & 0xFFF;
      unsigned int pomeraj = registri[b] + registri[c] + d;
      target = memory + pomeraj;
      memcpy(&registri[a], target, sizeof(registri[a]));
      break;
    }
    case 0x97:{ 
      //csr[A]<=mem32[gpr[B]]; gpr[B]<=gpr[B]+D;
      unsigned int a = (num >> 20) & 0xF; // e
      unsigned int b = (num >> 16) & 0xF; // f
      unsigned int c = (num >> 12) & 0xF; // 0
      unsigned int d = num & 0xFFF;
      target = memory + registri[b];
      memcpy(&registri[a], target, sizeof(registri[a]));
      //printf("pri dohvatanju %x sp je: %x\n", registri[a], registri[b]);
      registri[b] = registri[b] + d;
      break;
    }
    default:
      printf("Error in instructions\n");
      exit(-1);
      break;
    }
    
    // printf("Target: %p\n", target);
    //printf("%08x\n", num);
    
  }

  free(registri);
  munmap(memory, mem_size);
  
  close(fd);
}

int main(int argc, char* argv[]){
  if (argc != 2) {
    printf("Korišćenje: emulator <naziv_ulazne_datoteke>\n");
    return 1;
  }
  char *input_file = argv[1];
  //printf("Ulazna datoteka: %s\n", input_file);
  emulate(input_file);
  printf("Emulator finished.\n");
  return 0;
}