
%{
  #include "inc/assembler.h"
	#include <stdio.h>
  #include <stdlib.h>
  #include <string.h>
	void yyerror(const char*);
  int location_counter = 0;
  char* op = "";
  extern int yylex();
  extern int yyparse();
  extern FILE *yyin;
%}

%output "parser.c"
%defines "parser.h"

%union {
	int         num;
	char       *ident;
	struct arg *arg;
}

%token TOKEN_GLOBAL
%token TOKEN_EXTERN
%token TOKEN_WORD
%token TOKEN_SKIP
%token TOKEN_END
%token TOKEN_SECTION
%token TOKEN_COM
%token TOKEN_LPAR
%token TOKEN_RPAR
%token TOKEN_PLUS
%token TOKEN_SEMI
%token TOKEN_COMMA
%token TOKEN_DOT
%token TOKEN_REG
%token TOKEN_EOL
%token TOKEN_PERC
%token TOKEN_AMP
%token TOKEN_COL
%token TOKEN_HEX
%token TOKEN_HALT
%token TOKEN_INT
%token TOKEN_IRET
%token TOKEN_CALL
%token TOKEN_RET
%token TOKEN_JMP
%token TOKEN_BEQ
%token TOKEN_BNE
%token TOKEN_BGT
%token TOKEN_PUSH
%token TOKEN_POP
%token TOKEN_XCHG
%token TOKEN_ADD
%token TOKEN_SUB
%token TOKEN_MUL
%token TOKEN_DIV
%token TOKEN_NOT
%token TOKEN_AND
%token TOKEN_OR
%token TOKEN_XOR
%token TOKEN_SHL
%token TOKEN_SHR
%token TOKEN_LD
%token TOKEN_ST
%token TOKEN_CSRRD
%token TOKEN_CSRWR

%token <ident> TOKEN_R0
%token <ident> TOKEN_R1
%token <ident> TOKEN_R2
%token <ident> TOKEN_R3
%token <ident> TOKEN_R4
%token <ident> TOKEN_R5
%token <ident> TOKEN_R6
%token <ident> TOKEN_R7
%token <ident> TOKEN_R8
%token <ident> TOKEN_R9
%token <ident> TOKEN_R10
%token <ident> TOKEN_R11
%token <ident> TOKEN_R12
%token <ident> TOKEN_R13
%token <ident> TOKEN_R14
%token <ident> TOKEN_R15
%token <ident> TOKEN_RSP
%token <ident> TOKEN_RPC
%token <ident> TOKEN_RCAUSE
%token <ident> TOKEN_RSTATUS
%token <ident> TOKEN_RHANDLER

%token <ident> TOKEN_SYMBOL
%token <ident> TOKEN_LITERAL
%type <ident> operand
%type <ident> operand_jmp
%type <ident> reg
%type <ident> symbol_list
%type <ident> literal_list
%type <ident> lit_sym_list
%type <ident> csr
%type <ident> TOKEN_HEX
%type <ident> TOKEN_AMP
%type <ident> TOKEN_LPAR
%type <ident> TOKEN_RPAR
%token <num>   TOKEN_NUM


%%

prog
  :
  | instr prog | directive prog | label prog | TOKEN_COM TOKEN_EOL prog | TOKEN_EOL prog
  ;

label: symbol_list TOKEN_COL TOKEN_EOL{
  if(first_pass) label_first_pass($1);
  if(!first_pass) label_second_pass($1);
}
;

directive
  :
  TOKEN_GLOBAL symbol_list TOKEN_EOL{
    if(first_pass) global_directive_first_pass($2);
    if(!first_pass) global_directive_second_pass($2);
  } |
  TOKEN_EXTERN symbol_list TOKEN_EOL {
    if(first_pass) extern_directive_first_pass($2);
    if(!first_pass) extern_directive_second_pass($2);
  } |
  TOKEN_END TOKEN_EOL{
    if(!first_pass) end_directive_second_pass(NULL);
    if(first_pass) {
      location_counter = 0;
      first_pass = false;
      end_directive_first_pass(NULL);
    }
  } |
  TOKEN_WORD lit_sym_list TOKEN_EOL{
    if(first_pass) word_directive_first_pass($2);
    if(!first_pass) word_directive_second_pass($2);
  }|
  TOKEN_WORD symbol_list TOKEN_EOL{
    if(first_pass) word_directive_first_pass($2);
    if(!first_pass) word_directive_second_pass($2);
  }|
  TOKEN_SECTION symbol_list TOKEN_EOL{ 
    if(first_pass) {
      section_directive_first_pass($2);
    }
    if(!first_pass) section_directive_second_pass($2);
  }|
  TOKEN_SKIP TOKEN_LITERAL TOKEN_EOL
  {
    if(first_pass) skip_directive_first_pass($2);
    if(!first_pass) skip_directive_second_pass($2);
    int skip = atoi($2);
    if(first_pass) {
      for(int i=0; i<skip; i++){
        current_section->my_location_counter++;
      }
    }
  }
;

instr
  : 
   TOKEN_HALT TOKEN_EOL {
    if(first_pass) current_section->my_location_counter = current_section->my_location_counter+4;
    if(!first_pass) halt();
   }|
   TOKEN_HALT TOKEN_COM TOKEN_EOL {
    if(first_pass) current_section->my_location_counter = current_section->my_location_counter+4;
    if(!first_pass) halt();
   }
  | TOKEN_INT TOKEN_EOL {
    if(first_pass) current_section->my_location_counter= current_section->my_location_counter+4;
    if(!first_pass) iint();
  }
  | TOKEN_INT TOKEN_COM TOKEN_EOL {
    if(first_pass) current_section->my_location_counter = current_section->my_location_counter+4;
    if(!first_pass) iint();
  }
  | TOKEN_IRET TOKEN_EOL {
    if(first_pass)current_section->my_location_counter = current_section->my_location_counter+4;
    if(first_pass)current_section->my_location_counter = current_section->my_location_counter+4;
    if(!first_pass) iret();
  }
  | TOKEN_IRET TOKEN_COM TOKEN_EOL {
    if(first_pass)current_section->my_location_counter = current_section->my_location_counter+4;
    if(first_pass)current_section->my_location_counter = current_section->my_location_counter+4;
    if(!first_pass) iret();
  }
  | TOKEN_CALL operand_jmp TOKEN_EOL {
    if(first_pass)current_section->my_location_counter = current_section->my_location_counter+4;
    if(!first_pass) icall($2);
  }
  | TOKEN_CALL operand_jmp TOKEN_COM TOKEN_EOL {
    if(first_pass)current_section->my_location_counter = current_section->my_location_counter+4;
    if(!first_pass) icall($2);
  }
  | TOKEN_RET TOKEN_EOL {
    if(first_pass)current_section->my_location_counter = current_section->my_location_counter+4;
    if(!first_pass) rret();
  }
  | TOKEN_RET TOKEN_COM TOKEN_EOL {
    if(first_pass)current_section->my_location_counter = current_section->my_location_counter+4;
    if(!first_pass) rret();
  }
  | TOKEN_JMP operand_jmp TOKEN_EOL {
    if(first_pass)current_section->my_location_counter = current_section->my_location_counter+4;
    if(!first_pass) jmp($2);
  }
  | TOKEN_JMP operand_jmp TOKEN_COM TOKEN_EOL {
    if(first_pass)current_section->my_location_counter = current_section->my_location_counter+4;
    if(!first_pass) jmp($2);
  }
  | TOKEN_BEQ reg TOKEN_COMMA reg TOKEN_COMMA operand_jmp TOKEN_EOL {
    if(first_pass)current_section->my_location_counter = current_section->my_location_counter+4;
    if(!first_pass) beq($2+2, $4+2, $6);
  }
  | TOKEN_BEQ reg TOKEN_COMMA reg TOKEN_COMMA operand_jmp TOKEN_COM TOKEN_EOL {
    if(first_pass)current_section->my_location_counter = current_section->my_location_counter+4;
    if(!first_pass) beq($2+2, $4+2, $6);
  }
  | TOKEN_BNE reg TOKEN_COMMA reg TOKEN_COMMA operand_jmp TOKEN_EOL {
    if(first_pass)current_section->my_location_counter = current_section->my_location_counter+4;
    if(!first_pass) bne($2+2, $4+2, $6);
  }
  | TOKEN_BNE reg TOKEN_COMMA reg TOKEN_COMMA operand_jmp TOKEN_COM TOKEN_EOL {
    if(first_pass)current_section->my_location_counter = current_section->my_location_counter+4;
    if(!first_pass) bne($2+2, $4+2, $6);
  }
  | TOKEN_BGT reg TOKEN_COMMA reg TOKEN_COMMA operand_jmp TOKEN_EOL {
    if(first_pass)current_section->my_location_counter = current_section->my_location_counter+4;
    if(!first_pass) bgt($2+2, $4+2, $6);
  }
  | TOKEN_BGT reg TOKEN_COMMA reg TOKEN_COMMA operand_jmp TOKEN_COM TOKEN_EOL {
    if(first_pass)current_section->my_location_counter = current_section->my_location_counter+4;
    if(!first_pass) bgt($2+2, $4+2, $6);
  }
  | TOKEN_PUSH reg TOKEN_COM TOKEN_EOL {
    if(first_pass)current_section->my_location_counter = current_section->my_location_counter+4;
    if(!first_pass) {
      push($2+2);
    }
  }
  | TOKEN_PUSH reg TOKEN_EOL {
    if(first_pass)current_section->my_location_counter = current_section->my_location_counter+4;
    if(!first_pass) {
      push($2+2);
    }
  }
  | TOKEN_POP reg TOKEN_EOL {
    if(first_pass)current_section->my_location_counter = current_section->my_location_counter+4;
    if(!first_pass) {
      pop($2+2);
    }
  }
  | TOKEN_POP reg TOKEN_COM TOKEN_EOL {
    if(first_pass)current_section->my_location_counter = current_section->my_location_counter+4;
    if(!first_pass) {
      pop($2+2);
    }
  }
  | TOKEN_XCHG reg TOKEN_COMMA reg TOKEN_EOL {
    if(first_pass)current_section->my_location_counter = current_section->my_location_counter+4;
    if(!first_pass) xchg($2+2, $4+2);
  }
  | TOKEN_XCHG reg TOKEN_COMMA reg TOKEN_COM TOKEN_EOL {
    if(first_pass)current_section->my_location_counter = current_section->my_location_counter+4;
    if(!first_pass) xchg($2+2, $4+2);
  }
  | TOKEN_ADD reg TOKEN_COMMA reg TOKEN_EOL {
    if(first_pass)current_section->my_location_counter = current_section->my_location_counter+4;
    if(!first_pass) add($2+2, $4+2);
  }
  | TOKEN_SUB reg TOKEN_COMMA reg TOKEN_EOL {
    if(first_pass)current_section->my_location_counter = current_section->my_location_counter+4;
    if(!first_pass) sub($2+2, $4+2); /*trebalo bi ovo da posalje samo broj registra*/
  }
  | TOKEN_MUL reg TOKEN_COMMA reg TOKEN_EOL {
    if(first_pass)current_section->my_location_counter = current_section->my_location_counter+4;
    if(!first_pass) mul($2+2, $4+2);
  }
  | TOKEN_DIV reg TOKEN_COMMA reg TOKEN_EOL {
    if(first_pass)current_section->my_location_counter = current_section->my_location_counter+4;
    if(!first_pass) ddiv($2+2, $4+2);
  }
  | TOKEN_NOT reg TOKEN_EOL {
    if(first_pass)current_section->my_location_counter = current_section->my_location_counter+4;
    if(!first_pass) not($2+2);
  }
  | TOKEN_AND reg TOKEN_COMMA reg TOKEN_EOL {
    if(first_pass)current_section->my_location_counter = current_section->my_location_counter+4;
    if(!first_pass) and($2+2, $4+2);
  }
  | TOKEN_OR reg TOKEN_COMMA reg TOKEN_EOL {
    if(first_pass)current_section->my_location_counter = current_section->my_location_counter+4;
    if(!first_pass) or($2+2, $4+2);
  }
  | TOKEN_XOR reg TOKEN_COMMA reg TOKEN_EOL {
    if(first_pass)current_section->my_location_counter = current_section->my_location_counter+4;
    if(!first_pass) xor($2+2, $4+2);
  }
  | TOKEN_SHL reg TOKEN_COMMA reg TOKEN_EOL {
    if(first_pass)current_section->my_location_counter = current_section->my_location_counter+4;
    if(!first_pass) shl($2+2, $4+2);
  }
  | TOKEN_SHR reg TOKEN_COMMA reg TOKEN_EOL {
    if(first_pass)current_section->my_location_counter = current_section->my_location_counter+4;
    if(!first_pass) shr($2+2, $4+2);
  }
  | TOKEN_LD operand TOKEN_COMMA reg TOKEN_EOL {
    if(first_pass){
      current_section->my_location_counter = current_section->my_location_counter+4;
      if(strcmp(op, "literalmem")==0) current_section->my_location_counter = current_section->my_location_counter+4;
      if(strcmp(op, "simbolmem")==0) current_section->my_location_counter = current_section->my_location_counter+4;
    }
    if(!first_pass) {
      ld($2, $4+2, op);
    }
  }
  | TOKEN_ST reg TOKEN_COMMA operand TOKEN_EOL {
    if(first_pass){
      current_section->my_location_counter = current_section->my_location_counter+4;
      if(strcmp(op, "literalmem")==0) current_section->my_location_counter = current_section->my_location_counter+4;
    }
    if(!first_pass) st($4, $2+2, op);
  }
  | TOKEN_CSRRD csr TOKEN_COMMA reg TOKEN_EOL {
    if(first_pass)current_section->my_location_counter = current_section->my_location_counter+4;
    if(!first_pass) {
      instruction_cssrd($2+1, $4+2);
    }
  }
  | TOKEN_CSRWR reg TOKEN_COMMA csr TOKEN_EOL {
    if(first_pass)current_section->my_location_counter = current_section->my_location_counter+4;
    if(!first_pass) csrwr($2+2, $4+1);
  }| TOKEN_ADD reg TOKEN_COMMA reg TOKEN_COM TOKEN_EOL {
    if(first_pass)current_section->my_location_counter = current_section->my_location_counter+4;
    if(!first_pass) add($2+2, $4+2);
  }
  | TOKEN_SUB reg TOKEN_COMMA reg TOKEN_COM TOKEN_EOL {
    if(first_pass)current_section->my_location_counter = current_section->my_location_counter+4;
    if(!first_pass) sub($2+2, $4+2); /*trebalo bi ovo da posalje samo broj registra*/
  }
  | TOKEN_MUL reg TOKEN_COMMA reg TOKEN_COM TOKEN_EOL {
    if(first_pass)current_section->my_location_counter = current_section->my_location_counter+4;
    if(!first_pass) mul($2+2, $4+2);
  }
  | TOKEN_DIV reg TOKEN_COMMA reg TOKEN_COM TOKEN_EOL {
    if(first_pass)current_section->my_location_counter = current_section->my_location_counter+4;
    if(!first_pass) ddiv($2+2, $4+2);
  }
  | TOKEN_NOT reg TOKEN_COM TOKEN_EOL {
    if(first_pass)current_section->my_location_counter = current_section->my_location_counter+4;
    if(!first_pass) not($2+2);
  }
  | TOKEN_AND reg TOKEN_COMMA reg TOKEN_COM TOKEN_EOL {
    if(first_pass)current_section->my_location_counter = current_section->my_location_counter+4;
    if(!first_pass) and($2+2, $4+2);
  }
  | TOKEN_OR reg TOKEN_COMMA reg TOKEN_COM TOKEN_EOL {
    if(first_pass)current_section->my_location_counter = current_section->my_location_counter+4;
    if(!first_pass) or($2+2, $4+2);
  }
  | TOKEN_XOR reg TOKEN_COMMA reg TOKEN_COM TOKEN_EOL {
    if(first_pass)current_section->my_location_counter = current_section->my_location_counter+4;
    if(!first_pass) xor($2+2, $4+2);
  }
  | TOKEN_SHL reg TOKEN_COMMA reg TOKEN_COM TOKEN_EOL {
    if(first_pass)current_section->my_location_counter = current_section->my_location_counter+4;
    if(!first_pass) shl($2+2, $4+2);
  }
  | TOKEN_SHR reg TOKEN_COMMA reg TOKEN_COM TOKEN_EOL {
    if(first_pass)current_section->my_location_counter = current_section->my_location_counter+4;
    if(!first_pass) shr($2+2, $4+2);
  }
  | TOKEN_LD operand TOKEN_COMMA reg TOKEN_COM TOKEN_EOL {
    if(first_pass){
      current_section->my_location_counter = current_section->my_location_counter+4;
      if(strcmp(op, "literalmem")==0) current_section->my_location_counter = current_section->my_location_counter+4;
      if(strcmp(op, "simbolmem")==0) current_section->my_location_counter = current_section->my_location_counter+4;
    }
    if(!first_pass) {
      ld($2, $4+2, op);
    }
  }
  | TOKEN_ST reg TOKEN_COMMA operand TOKEN_COM TOKEN_EOL {
    if(first_pass){
      current_section->my_location_counter = current_section->my_location_counter+4;
      if(strcmp(op, "literalmem")==0) current_section->my_location_counter = current_section->my_location_counter+4;
    }
    if(!first_pass) st($4, $2+2, op);
  }
  | TOKEN_CSRRD csr TOKEN_COMMA reg TOKEN_COM TOKEN_EOL {
    if(first_pass)current_section->my_location_counter = current_section->my_location_counter+4;
    if(!first_pass) {
      instruction_cssrd($2+1, $4+2);
    }
  }
  | TOKEN_CSRWR reg TOKEN_COMMA csr TOKEN_COM TOKEN_EOL {
    if(first_pass)current_section->my_location_counter = current_section->my_location_counter+4;
    if(!first_pass) csrwr($2+2, $4+1);
  }
  ;

reg: 
TOKEN_R0 { $$ = $1; } | 
TOKEN_R1 { $$ = $1; }| 
TOKEN_R2 { $$ = $1; }| 
TOKEN_R3 { $$ = $1; }| 
TOKEN_R4 { $$ = $1; }| 
TOKEN_R5 { $$ = $1; }| 
TOKEN_R6 { $$ = $1; }| 
TOKEN_R7 { $$ = $1; }| 
TOKEN_R8 { $$ = $1; }| 
TOKEN_R9 { $$ = $1; }| 
TOKEN_R10 { $$ = $1; }| 
TOKEN_R11 { $$ = $1; }| 
TOKEN_R12 { $$ = $1; }| 
TOKEN_R13 { $$ = $1; }| 
TOKEN_R14 { $$ = $1; }| 
TOKEN_R15 { $$ = $1; }|
TOKEN_RSP { $$ = $1; }|
TOKEN_RPC { $$ = $1; }
;

csr:
 TOKEN_RCAUSE { $$ = $1; }
 | TOKEN_RSTATUS { $$ = $1; }
 | TOKEN_RHANDLER { $$ = $1; }
 ;

operand:
  TOKEN_AMP TOKEN_SYMBOL {
    $$ = malloc(strlen($1) + strlen($2) + 1); 
    strcpy($$, $1+1);
    op = "simbol";
  } |
  TOKEN_AMP TOKEN_LITERAL {
    $$ = malloc(strlen($1) + strlen($2) + 1); 
    strcpy($$, $1+1);
    op = "literal";
  } |
  TOKEN_AMP TOKEN_HEX {
    $$ = malloc(strlen($1) + strlen($2) + 1); 
    strcpy($$, $1+1);
    op = "literal";
  } |
  TOKEN_SYMBOL {
    $$ = $1;
    op = "simbolmem";
  } |
  TOKEN_LITERAL {
    $$ = $1;
    op = "literalmem";
  } |
  reg {op="reg";}|
  TOKEN_LPAR reg TOKEN_RPAR {
    $$ = malloc(strlen($1) + strlen($2) + 1);
    strcpy($$, $1);
    op = "regmem";
  } |
  TOKEN_LPAR reg TOKEN_PLUS TOKEN_SYMBOL TOKEN_RPAR {
    $$ = malloc(strlen($2) + strlen($4) + 3);
    strcpy($$, $1);
    op="regsim";
  } |
  TOKEN_LPAR reg TOKEN_PLUS literal_list TOKEN_RPAR {
    $$ = malloc(strlen($2) + strlen($4) + 3);
    strcpy($$, $1);
    op="reglit";
  }
;

operand_jmp:
  TOKEN_SYMBOL {
    $$ = $1;
  }|
  TOKEN_HEX {
    $$ = malloc(strlen($1) + 1);
    strcpy($$, $1);
  }|
  TOKEN_LITERAL {
    $$ = $1;
  } ;

symbol_list:
  TOKEN_SYMBOL {
    $$ = malloc(strlen($1)+1);
    strcpy($$, $1);
  }
  |
  symbol_list TOKEN_COMMA TOKEN_SYMBOL {
    $$ = malloc(strlen($1) + strlen($3) + 2);
    strcpy($$, $1);
    strcat($$, ",");
    strcat($$, $3);
  }
;

literal_list:
  TOKEN_LITERAL {
    $$ = malloc(strlen($1) + 1);
    strcpy($$, $1);
  } |
  TOKEN_HEX {
    $$ = malloc(strlen($1) + 1);
    strcpy($$, $1);
  }
  |
  literal_list TOKEN_COMMA TOKEN_LITERAL {
    $$ = malloc(strlen($1) + strlen($3) + 2);
    strcpy($$, $1);
    strcat($$, ",");
    strcat($$, $3);
  }
  |
  literal_list TOKEN_COMMA TOKEN_HEX {
    $$ = malloc(strlen($1) + strlen($3) + 2);
    strcpy($$, $1);
    strcat($$, ",");
    strcat($$, $3);
  }
;

lit_sym_list:
  TOKEN_LITERAL {
    $$ = malloc(strlen($1) + 1);
    strcpy($$, $1);
  } |
  TOKEN_HEX {
    $$ = malloc(strlen($1) + 1);
    strcpy($$, $1);
  }
  |
  lit_sym_list TOKEN_COMMA TOKEN_LITERAL {
    $$ = malloc(strlen($1) + strlen($3) + 2);
    strcpy($$, $1);
    strcat($$, ",");
    strcat($$, $3);
  }
  |
  lit_sym_list TOKEN_COMMA TOKEN_HEX {
    $$ = malloc(strlen($1) + strlen($3) + 2);
    strcpy($$, $1);
    strcat($$, ",");
    strcat($$, $3);
  } 
  |
  lit_sym_list TOKEN_COMMA TOKEN_SYMBOL {
    $$ = malloc(strlen($1) + strlen($3) + 2);
    strcpy($$, $1);
    strcat($$, ",");
    strcat($$, $3);
  }
;

%%