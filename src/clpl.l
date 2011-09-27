
%option prefix="clpl_"
%option outfile="lex.yy.c" header-file="clpl_scanner.h"
%option noyywrap
%option 8bit
%option bison-bridge
%option bison-locations
%option yylineno
%option   warn nodefault
%option nounput
%start C_COMMENT


%{
#include "common.h"
#include "fresh_constants.h"
#include "rete.h"
#include "clpl_parser.h"


extern int fileno(FILE*);
extern int clpl_lineno;
%}


digit         [0-9]
Letter         [A-Z]
letter        [a-zA-Z]
symbol     [\'\"_]


%%
<INITIAL>"/*"                           {  BEGIN(C_COMMENT);}
<C_COMMENT>"*/"                BEGIN(INITIAL);
<C_COMMENT>.                     { ;}
<INITIAL>,                                            {  return COMMA;                   }
<INITIAL>;                                    {  return OR_SEPARATOR;  }
<INITIAL>\(                                         {  return LPAREN;                  }
<INITIAL>\)                                         {  return RPAREN;                 }
<INITIAL>"."                                         {  return PERIOD;                   }
<INITIAL>=>                                        {  return ARROW;                   }
<INITIAL>"_"                             { return UNDERSCORE; }
<INITIAL>"/"                             { return SLASH; }
<INITIAL>":"                                { return COLON; }
<INITIAL>"-"                                { return DASH; }
<INITIAL>\[                                         {  return LSQPAREN;                  }
<INITIAL>\]                                         {  return RSQPAREN;                 }
<INITIAL>\|                                          { return HOR_BAR; }
<INITIAL>"next"                                { return STRING_NEXT;                      }
<INITIAL>"enabled"                                { return STRING_ENABLED;                      }
<INITIAL>"true"                                  {  return TRUE;                       }
<INITIAL>"name"                                  {  return STRING_NAME;                       }
<INITIAL>"dom"                                  {  return STRING_DOM;                       }
<INITIAL>"false"                                 {  return FALSE;                     }
<INITIAL>"goal"                                  {  return GOAL;                       }
<INITIAL>"axiom"                                  { return AXIOM_NAME; }
<INITIAL>"dynamic"                                { return DYNAMIC;                      }
<INITIAL>{Letter}({letter}|{digit}|{symbol})* {
                                               yylval->str = malloc (strlen(yytext)+1);
                                               strcpy(yylval->str, yytext);
                                              return VARIABLE;                   }
<INITIAL>(\'[^\']*\')|({letter}|{digit}|{symbol}|"["|"]")+    {
                                               yylval->str = malloc (strlen(yytext)+1);
                                               strcpy(yylval->str, yytext);
                                              return NAME;                   }
<INITIAL>{digit}+                                { yylval->str = malloc (strlen(yytext)+1);
                                               strcpy(yylval->str, yytext);
                                               return INTEGER; }
[ \t]                            {                                                  }
[\n\r]                            {                                               }
%[^\n]*$                        {                                                  }
<INITIAL>.                                           { fprintf( stderr, "Error with symbol %s\n", yytext); exit( EXIT_FAILURE ); }
%%

