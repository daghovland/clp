
/* geolog_parser.y

   Copyright 2011 ?

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc.,
   51 Franklin Street - Fifth Floor, Boston, MA  02110-1301, USA */

/*   Written 2011 by Dag Hovland, hovlanddag@gmail.com  */


%{

#include "common.h"
#include "term.h"
#include "atom.h"
#include "constants.h"
#include "conjunction.h"
#include "disjunction.h"
#include "axiom.h"
#include "theory.h"
#include "parser.h"

  extern int geolog_lex();
  extern int fileno(FILE*);
  
  //  void geolog_error(char*);
  theory *th;
  
  extern FILE* geolog_in;
  extern int geolog_lineno;

  

%}

%define api.pure
%name-prefix "geolog_"
%error-verbose
%locations
%glr-parser
%expect 0

%union{
  char* str;
  clp_atom* atom;
  clp_term* term;
  term_list* terms;
  clp_conjunction* conj;
  clp_disjunction* disj;
  clp_axiom* axiom;
  theory* theory;
}

%token <str> VARIABLE
%token <str> NAME
%token LPAREN
%token RPAREN    
%left OR_SEPARATOR
%left COMMA
%token TRUE
%token ARROW
%token NEWLINE
%token PERIOD
%token FALSE
%token GOAL
%token DYNAMIC
%token AXIOM_NAME
%token UNDERSCORE
%token <str> INTEGER
%token COLON
%token SLASH
%token DASH
%token SECTION_MARKER
%token COMMENT


%type <atom> atom
%type <term> term
%type <terms> terms
%type <disj> disjunction
%type <conj> conjunction
%type <axiom> axiom


%start file

%{

void geolog_error(YYLTYPE*, char const *);				   

 %}

%%
file:          theory { ; }
;
theory:      theory_line theory {;}
                | theory_line {;}

theory_line:    axiom PERIOD { extend_theory(th, $1); }
                     | COMMENT {;}
;
axiom:    TRUE ARROW disjunction  {$$ = create_fact($3, th); }
| ARROW disjunction   {$$ = create_fact($2, th); }
| disjunction   {$$ = create_fact($1, th); }
              | conjunction ARROW GOAL   { $$ = create_goal($1);}
              | conjunction ARROW FALSE  { $$ = create_goal($1);}
              | conjunction ARROW disjunction   { $$ = create_axiom($1, $3, th);}
               ;
conjunction:   atom { $$ = create_conjunction($1);}
                         | conjunction COMMA TRUE { $$ = $1;}
                         | TRUE COMMA conjunction { $$ = $3;}
                         | conjunction COMMA atom { $$ = extend_conjunction($1, $3);}
                          ;
atom:      NAME LPAREN RPAREN { $$ = create_prop_variable($1, th); }
           | NAME LPAREN terms RPAREN {$$ = parser_create_atom($1, $3, th);}
           | NAME {$$ = create_prop_variable($1, th);}
               ;
terms:         terms COMMA term
                    { $$ = extend_term_list($1, $3); }
                   | term 
                   { $$ = create_term_list($1); }
                   ;
term:         NAME LPAREN terms RPAREN 
                   { $$ = create_function_term($1, $3); }
                   | NAME
                   { $$ = parser_create_constant_term(th, $1); }
                   | INTEGER
                   { $$ = parser_create_constant_term(th, $1); }
                   | VARIABLE 
                   { $$ = create_variable($1, th); }
                   ;
disjunction:            conjunction  { $$ = create_disjunction($1);}
                                 | disjunction OR_SEPARATOR conjunction { $$ = extend_disjunction($1, $3);}
                                 | disjunction OR_SEPARATOR FALSE { $$ = $1;}
                                 | FALSE OR_SEPARATOR disjunction { $$ = $3;}
;
%%


void geolog_error(YYLTYPE* locp, char const * s){
  fprintf(stderr, "Error in parsing file: %s",s);
  fprintf(stderr, " line  %d\n", geolog_lineno);
  exit(2);
}

theory* geolog_parser(FILE* f){
  th = create_theory();
  geolog_in = f;
  geolog_lineno = 1;
  geolog_parse();
  finalize_theory(th);
  return th;
}
