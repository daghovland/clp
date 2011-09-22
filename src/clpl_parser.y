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
/**
   Parses the format used by CL.pl
**/

%{


#include "common.h"
#include "term.h"
#include "atom.h"
#include "conjunction.h"
#include "disjunction.h"
#include "axiom.h"
#include "theory.h"
#include "parser.h"

 

  void clpl_error(char*);
  int clpl_lex();
  
  theory *th;

  char* theory_name = NULL;

  extern FILE* clpl_in;
  
  extern int fileno(FILE*);
  extern int clpl_lineno;

%}

%define api.pure
%name-prefix "clpl_"
%locations
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
%token STRING_NAME
%token STRING_DOM
%token STRING_NEXT
%token STRING_ENABLED
%token SECTION_MARKER
%token LSQPAREN
%token RSQPAREN
%left HOR_BAR
%token GOAL
%token DYNAMIC
%token AXIOM_NAME
%token UNDERSCORE
%token <str> INTEGER
%token COLON
%token SLASH
%token DASH
%token COMMENT


%type <atom> atom
%type <term> term
%type <terms> terms
%type <disj> disjunction
%type <conj> conjunction
%type <axiom> axiom
%type <axiom> axiomw
%type <axiom> domain_line
%type <str> axiom_name
%start theory



%%
theory_name_line:    STRING_NAME LPAREN NAME RPAREN PERIOD { set_theory_name(th, $3); }
;
dynamicline:  COLON DASH DYNAMIC predlist PERIOD  {;} // :- dynamic e/2,r/3 ...
;
domain_line: STRING_DOM LPAREN NAME RPAREN PERIOD {$$ = create_fact(create_disjunction(create_conjunction(create_dom_atom(parser_create_constant_term(th, $3),th))));} // dom(constant).
;
theory:   STRING_NEXT {;}
                | STRING_ENABLED {;}
                | SECTION_MARKER {;}
                | theory_line {;}
                | theory_line theory {;}

;
prolog_atom_list : prolog_atom COMMA prolog_atom_list { ; }
                   | prolog_atom {;}
;
prolog_atom :  NAME LPAREN RPAREN {;}
             | NAME LPAREN prolog_arg_list RPAREN {;}
;
prolog_arg_list:     prolog_arg 
                   | prolog_arg COMMA prolog_arg_list {;}
;
prolog_arg:         UNDERSCORE {;}
                          | VARIABLE {;}
                          | LSQPAREN prolog_list_content RSQPAREN { ; }
                          | LSQPAREN RSQPAREN { ; }
                          | NAME {;}
                          | INTEGER {;}
                          | NAME LPAREN prolog_arg_list RPAREN {;}
;
prolog_list_content : VARIABLE HOR_BAR VARIABLE {;}
                      | prolog_arg { ; }
                      | prolog_list_content COMMA prolog_arg {;}
;
theory_line:    axiomw { extend_theory(th, $1); }
               | strategy_def { ; }
               | domain_line { extend_theory(th, $1);}
               | theory_name_line { ; }
               | dynamicline { ; }
               | COMMENT { ; }
;
axiom_name:    NAME LPAREN varlist RPAREN { $$ = $1;}
                      | NAME LPAREN RPAREN { $$ = $1;}
                      | NAME { $$ = $1;}
;
axmno:     NAME {;}
| UNDERSCORE {;}
;
predlist:   NAME SLASH NAME {;}
           | NAME SLASH NAME COMMA predlist {;}
;
axiomw:     axmno AXIOM_NAME axiom_name COLON axiom PERIOD { set_axiom_name($5,$3), $$=$5 ; }
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
          | STRING_DOM LPAREN VARIABLE RPAREN { $$ = create_dom_atom(create_variable($3, th),th); }
          | STRING_DOM LPAREN NAME RPAREN { $$ = create_dom_atom(parser_create_constant_term(th, $3),th); }
          | NAME LPAREN terms RPAREN {$$ = parser_create_atom($1, $3, th);}
          | NAME {$$ = create_prop_variable($1, th);}
               ;
terms:         terms COMMA term
                    { $$ = extend_term_list($1, $3); }
                   | term 
                   { $$ = create_term_list($1); }
                   ;
term:         INTEGER
                   { $$ = parser_create_constant_term(th, $1); }
                   | NAME LPAREN terms RPAREN 
                   { $$ = create_term($1, $3); }
                   | NAME
                   { $$ = parser_create_constant_term(th, $1); }
                   | VARIABLE 
                   { $$ = create_variable($1, th); }
                   ;
disjunction:            conjunction  { $$ = create_disjunction($1);}
                                 | disjunction OR_SEPARATOR conjunction { $$ = extend_disjunction($1, $3);}
;

strategy_def:         STRING_NEXT LPAREN prolog_arg COMMA prolog_arg COMMA prolog_arg RPAREN PERIOD { ; }
                      | STRING_ENABLED LPAREN prolog_arg COMMA prolog_arg RPAREN PERIOD { ; }
                      | STRING_ENABLED LPAREN prolog_arg COMMA prolog_arg RPAREN COLON DASH prolog_atom_list PERIOD { ; }
;
%%

				   

void clpl_error(char* s){
  fprintf(stderr, "Error in parsing file: %s",s);
  fprintf(stderr, " line  %d\n", clpl_lineno);
  exit(2);
}

theory* clpl_parser(FILE* f){
  th = create_theory();
  clpl_in = f;
  clpl_lineno = 1;
  clpl_parse();
  return th;
}
