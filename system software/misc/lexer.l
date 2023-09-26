%{
#include "parser.h"
#include "inc/assembler.h"


%}

%option outfile="lexer.c" header-file="lexer.h"

%option noyywrap


GLOBAL ".global"
EXTERN ".extern"
SECTION ".section"
WORD ".word"
SKIP ".skip"
END ".end"

R0 "%r0"
R1 "%r1"
R2 "%r2"
R3 "%r3"
R4 "%r4"
R5 "%r5"
R6 "%r6"
R7 "%r7"
R8 "%r8"
R9 "%r9"
R10 "%r10"
R11 "%r11"
R12 "%r12"
R13 "%r13"
R14 "%r14"
RSP "%sp"
RPC "%pc"
R15 "%r15"
RCAUSE "%cause"
RSTATUS "%status"
RHANDLER "%handler"


HALT "halt"
INT "int"
IRET "iret"
CALL "call"
RET "ret"
JMP "jmp"
BEQ "beq"
BNE "bne"
BGT "bgt"
PUSH "push"
POP "pop"
XCHG "xchg"
ADD "add"
SUB "sub"
MUL "mul"
DIV "div"
NOT "not"
AND "and"
OR "or"
XOR "xor"
SHL "shl"
SHR "shr"
LD "ld"
ST "st"
CSRRD "csrrd"
CSRWR "csrwr"

LPAR  "["
RPAR  "]"
PLUS  "+"
SEMI  ";"
COMMA ","
DOT "\."
PERC "%"

AMP "$"
COL ":"

HEX   0[Xx][0-9A-Fa-f]+
COM   "#"[^\n\r]*
EOL [\r\n]+
WS    [ \t]+
SYMBOL [A-Za-z_][A-Za-z0-9_]* 
LITERAL [0-9]*


%%
{HALT} {return TOKEN_HALT;}
{INT} {return TOKEN_INT;}
{IRET} {return TOKEN_IRET;}
{CALL} {return TOKEN_CALL;}
{RET} {return TOKEN_RET;}
{JMP} {return TOKEN_JMP;}
{BEQ} {return TOKEN_BEQ;}
{BNE} {return TOKEN_BNE;}
{BGT} {return TOKEN_BGT;}
{PUSH} {return TOKEN_PUSH;}
{POP} {return TOKEN_POP;}
{XCHG} {return TOKEN_XCHG;}
{ADD} {return TOKEN_ADD;}
{SUB} {return TOKEN_SUB;}
{MUL} {return TOKEN_MUL;}
{DIV} {return TOKEN_DIV;}
{NOT} {return TOKEN_NOT;}
{AND} {return TOKEN_AND;}
{OR} {return TOKEN_OR;}
{XOR} {return TOKEN_XOR;}
{SHL} {return TOKEN_SHL;}
{SHR} {return TOKEN_SHR;}
{LD} {return TOKEN_LD;}
{ST} {return TOKEN_ST;}
{CSRRD} {return TOKEN_CSRRD;}
{CSRWR} {return TOKEN_CSRWR;}

{WS}        {
    /* skip whitspace */ 
}
{COM}       { 
    return TOKEN_COM;
    /* also skip comments */ }
{HEX}		{
	          sscanf(yytext, "%x", &yylval.num);
              yylval.ident = yytext;
			  return TOKEN_HEX;
            }


{GLOBAL} {return TOKEN_GLOBAL; }
{EXTERN} {return TOKEN_EXTERN; }
{SKIP} {return TOKEN_SKIP; }
{SECTION} {return TOKEN_SECTION; }
{WORD} {return TOKEN_WORD; }
{END} {return TOKEN_END; }

{R0} {
    yylval.ident = yytext;
    return TOKEN_R0; }
{R1} { yylval.ident = yytext; return TOKEN_R1; }
{R2} { yylval.ident = yytext; return TOKEN_R2; }
{R3} {yylval.ident = yytext; return TOKEN_R3; }
{R4} {yylval.ident = yytext; return TOKEN_R4; }
{R5} {yylval.ident = yytext; return TOKEN_R5; }
{R6} {yylval.ident = yytext; return TOKEN_R6; }
{R7} {yylval.ident = yytext; return TOKEN_R7; }
{R8} {yylval.ident = yytext; return TOKEN_R8; }
{R9} {yylval.ident = yytext; return TOKEN_R9; }
{R10} {yylval.ident = yytext; return TOKEN_R10; }
{R11} {yylval.ident = yytext; return TOKEN_R11; }
{R12} {yylval.ident = yytext; return TOKEN_R12; }
{R13} {yylval.ident = yytext; return TOKEN_R13; }
{R14} {yylval.ident = yytext; return TOKEN_R14; }
{R15} {yylval.ident = yytext; return TOKEN_R15; }
{RSP} {yylval.ident = yytext; return TOKEN_RSP; }
{RPC} {yylval.ident = yytext; return TOKEN_RPC; }
{RCAUSE} {yylval.ident = yytext; return TOKEN_RCAUSE; }
{RSTATUS} {yylval.ident = yytext; return TOKEN_RSTATUS; }
{RHANDLER} {yylval.ident = yytext; return TOKEN_RHANDLER; }

{LITERAL}     {
	          yylval.ident = yytext;
			  return TOKEN_LITERAL;
            }
{SYMBOL}     {
	          yylval.ident = yytext;
			  return TOKEN_SYMBOL;
            }


{LPAR}      { yylval.ident = yytext; return TOKEN_LPAR; }
{RPAR}      { yylval.ident = yytext; return TOKEN_RPAR; }
{PLUS}      { return TOKEN_PLUS; }
{SEMI}      { return TOKEN_SEMI; }
{COMMA}     { 
    return TOKEN_COMMA; }
{DOT}       { return TOKEN_DOT; }
{EOL}       { return TOKEN_EOL; }
{PERC} {return TOKEN_PERC; }

{COL} {return TOKEN_COL;}
{AMP} {
    yylval.ident = yytext;
    return TOKEN_AMP;}

%%

int
yyerror(const char* msg)
{
    fprintf(stderr, "lexer error: %s\n", msg);
    return 0;
}
