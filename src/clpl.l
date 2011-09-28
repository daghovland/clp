
%option prefix="clpl_"
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
<INITIAL>"="                                { return EQUALS; }
<INITIAL>"\\="                                { return NOT_EQUALS; }
<INITIAL>"*"                                { return TIMES; }
<INITIAL>\[                                         {  return LSQPAREN;                  }
<INITIAL>\]                                         {  return RSQPAREN;                 }
<INITIAL>\|                                          { return HOR_BAR; }
<INITIAL>"next"                                { return STRING_NEXT;                      }
<INITIAL>"enabled"                                { return STRING_ENABLED;                      }
<INITIAL>"true"                                  {  return TRUE;                       }
<INITIAL>"name"                                  {  return STRING_NAME;                       }
<INITIAL>"false"                                 {  return FALSE;                     }
<INITIAL>"goal"                                  {  return GOAL;                       }
<INITIAL>"axiom"                                  { return AXIOM_NAME; }
<INITIAL>"dynamic"                                { return DYNAMIC;                      }
<INITIAL>{Letter}({letter}|{digit}|{symbol})* {
                                               yylval->str = malloc (strlen(yytext)+1);
                                               strcpy(yylval->str, yytext);
                                              return VARIABLE;                   }
<INITIAL>(\'[^\']+\')|({digit}+"."{digit}+)|({letter}|{digit}|{symbol}|"["|"]")+    {
                                               yylval->str = malloc (strlen(yytext)+1);
                                               strcpy(yylval->str, yytext);
                                              return NAME;                   }
[ \t]                            {                                                  }
[\n\r]                            {                                               }
<INITIAL>"%"[^\n]*                        {                                                 }
<INITIAL>.                                           { fprintf( stderr, "Error with symbol %s\n", yytext); exit( EXIT_FAILURE ); }
%%

