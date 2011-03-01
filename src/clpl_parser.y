/* clpl_parser.y

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
#include "conjunction.h"
#include "disjunction.h"
#include "axiom.h"
#include "theory.h"
#include "parser.h"

  
  
  void yyerror(char*);
  int yylex();

  theory *th;

  extern int fileno(FILE*);
  int lineno = 1;
  
%}
%pure-parser
%name-prefix="clpl_"
%locations
%defines
%error-verbose

%union{
  char* str;
  atom* atom;
  term* term;
  term_list* terms;
  conjunction* conj;
  disjunction* disj;
  axiom* axiom;
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
%type <axiom> axiomw


%start file



%%
file:          theory { ; }
;
theory:      theory_line theory {;}
                | theory_line {;}
               | SECTION_MARKER prolog {;}
;
prolog:       prolog_line prolog {;}
               |  {;}
;
prolog_line:      prolog_atom PERIOD {;} // To be able to read CL.pl format
                       | prolog_atom COLON DASH prolog_atom_list PERIOD {;}
;
prolog_atom_list:       prolog_atom COMMA prolog_atom_list {;}
                                   | prolog_atom {;}
;
prolog_atom:          NAME {;}
                             | NAME LPAREN prolog_arg_list RPAREN {;}
;
prolog_arg_list:     prolog_arg COMMA prolog_arg_list {;}
                             | prolog_arg {;}
;
prolog_arg:         UNDERSCORE {;}
                          | VARIABLE {;}
                          | NAME {;}
                          | INTEGER {;}
                          | NAME LPAREN prolog_arg_list RPAREN {;}
;
theory_line:    axiomw { extend_theory(th, $1); }
                     |  COLON DASH DYNAMIC predlist PERIOD  {;} // :- dynamic e/2,r/3 ...
                     | COMMENT {;}
;
axmdef:    NAME LPAREN varlist RPAREN {;}
                      | NAME LPAREN RPAREN {;}
                      | NAME {;}
;
predlist:   NAME SLASH INTEGER {;}
           | NAME SLASH INTEGER COMMA predlist {;}
;
axiomw:     axiom PERIOD { $$ = $1; }
               | axmno AXIOM_NAME axmdef COLON axiom PERIOD { $$ = $5; }
;
axmno:      INTEGER {;} 
          | UNDERSCORE {;}
;
varlist:      VARIABLE { ; }
            |  VARIABLE COMMA varlist { ; }
;
axiom:    LPAREN axiom RPAREN { $$ = $2; }    
              |  TRUE ARROW disjunction  {$$ = create_fact($3); }
              | ARROW disjunction   {$$ = create_fact($2); }
              | disjunction   {$$ = create_fact($1); }
              | conjunction ARROW GOAL   { $$ = create_goal($1);}
              | conjunction ARROW FALSE  { $$ = create_goal($1);}
              | conjunction ARROW disjunction   { $$ = create_axiom($1, $3);}
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
                   { $$ = create_term($1, $3); }
                   | NAME
                   { $$ = create_constant($1); }
                   | INTEGER
                   { $$ = create_constant($1); }
                   | VARIABLE 
                   { $$ = create_variable($1, th); }
                   ;
disjunction:            conjunction  { $$ = create_disjunction($1);}
                                 | disjunction OR_SEPARATOR conjunction { $$ = extend_disjunction($1, $3);}
;
%%

				   

void yyerror(char* s){
  fprintf(stderr, "Error in parsing file: %s",s);
  fprintf(stderr, " line  %d\n", lineno);
  exit(2);
}

theory* clpl_parser(FILE* f){
  th = create_theory();
  yyparse();
  return th;
}
