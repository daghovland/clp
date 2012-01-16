
%option prefix="tptp_"
%option noyywrap
%option 8bit
%option bison-bridge
%option bison-locations
%option yylineno
%option   warn nodefault
%option nounput


%{
#include "common.h"
#include "fresh_constants.h"
#include "rete.h"
#include "tptp_parser.h"


extern int fileno(FILE*);
extern int tptp_lineno;
%}

comment                 comment_line|comment_block
comment_line            [%]{printable_char}*
comment_block           [/][*]{not_star_slash}[*][*]*[/]
not_star_slash        ([^*]*[*][*]*[^/*])*[^*]*
zero_numeric            0
non_zero_numeric        [1-9]
numeric                 [0-9]
upper_alpha             [A-Z]
lower_alpha             [a-z]
symbol                  [\'\"_]
lower_word            {lower_alpha}+
dollar_word           [$]{lower_word}
dollar_dollar_word    {dollar}{dollar}{lower_word}

%%
\.                                           { return PERIOD; }
","                                           { return COMMA; }
\*                                           { return STAR; }
"+"                                           { return PLUS; }
"-"                                           { return MINUS; }
"~"                                           { return TILDE; }
"^"                                           { return HAT; }
"="                                           { return EQUALS; }
"!"                                           { return EXCLAMATION; }
"?"                                           { return QUESTION; }
"$"                                           { return DOLLAR; }
">"                                          { return ARROW; }
"<"                                           { return LESS_SIGN; }
":"                                           { return COLON; }
\(                                            { return LPAREN; }
\)                                            { return RPAREN; }
\|                                             { return VLINE; }
\[                                         {  return LSQPAREN;                  }
\]                                         {  return RSQPAREN;                 }
"&"                                             { return AMPERSAND; }
"@"                                             { return ATSIGN; }
"thf"                                           { return thf_string; }
"ttf"                                           { return tff_string; }
"fof"                                           { return fof_string; }
"cnf"                                           { return cnf_string; }
"include"                                           { return include_string; }
"true"                                           { return true_string; }
"false"                                           { return false_string; }
.                                           { return printable_char;}
%%

