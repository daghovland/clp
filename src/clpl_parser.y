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
%glr-parser
%expect-rr 1
%expect 1


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
%token STRING_NAME
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
%token COLON
%token SLASH
%token EQUALS
%token NOT_EQUALS
%token TIMES
%token DASH



%type <atom> atom
%type <term> term
%type <terms> terms
%type <disj> disjunction
%type <conj> conjunction
%type <axiom> axiom
%type <axiom> axiomw
%type <str> axiom_name
%start theory

%{
  void clpl_error(YYLTYPE*, char const*);
  %}

%%

theory:   theory_line {;}
        | theory_line theory {;}
;
theory_name_line:    STRING_NAME LPAREN NAME RPAREN PERIOD { set_theory_name(th, $3); }
;
dynamicline:  COLON DASH DYNAMIC predlist PERIOD  {;} // :- dynamic e/2,r/3 ...
;
redefine_axiom_prolog:     LPAREN UNDERSCORE AXIOM_NAME LPAREN prolog_arg COLON VARIABLE RPAREN RPAREN COLON DASH VARIABLE PERIOD {;}
;
theory_line:   redefine_axiom_prolog {;}
               | axiomw { extend_theory(th, $1); }
               | strategy_def { ; }
               | theory_name_line { ; }
               | dynamicline { ; }
;
axiomw:               axiom PERIOD { $$ = $1;}
|   axiom_name AXIOM_NAME axiom_name COLON axiom PERIOD { set_axiom_name($5,$3), $$=$5 ; }
|     UNDERSCORE AXIOM_NAME axiom_name COLON axiom PERIOD { set_axiom_name($5,$3), $$=$5 ; }
;


axiom_name:    NAME LPAREN varlist RPAREN { $$ = $1;}
                      | NAME LPAREN RPAREN { $$ = $1;}
                      | NAME { $$ = $1;}
;
predicate_def : NAME SLASH NAME {;}
;
predlist:   predicate_def COMMA predlist {;}
           | predicate_def  {;}
;

varlist:      VARIABLE { ; }
            |  VARIABLE COMMA varlist { ; }
;
axiom:    LPAREN axiom RPAREN { $$ = $2; }    
              |  TRUE ARROW disjunction  {$$ = create_fact($3, th); }
              | ARROW disjunction   {$$ = create_fact($2, th); }
              | disjunction   {$$ = create_fact($1, th); }
              | conjunction ARROW disjunction   { $$ = create_axiom($1, $3, th);}
              | conjunction ARROW GOAL   { $$ = create_goal($1);}
              | conjunction ARROW FALSE  { $$ = create_goal($1);}
;
conjunction:   atom { $$ = create_conjunction($1);}
                         | conjunction COMMA TRUE { $$ = $1;}
                         | TRUE COMMA conjunction { $$ = $3;}
                         | conjunction COMMA atom { $$ = extend_conjunction($1, $3);}
;
atom:      term EQUALS term { $$ = parser_create_equality($1, $3, th);} 
          | EQUALS LPAREN term COMMA term RPAREN { $$ = parser_create_equality($3, $5, th);} 
          | NAME LPAREN RPAREN { $$ = create_prop_variable($1, th); }
          | NAME LPAREN terms RPAREN {$$ = parser_create_atom($1, $3, th);}
          | NAME {$$ = create_prop_variable($1, th);}
;
terms:         terms COMMA term
                    { $$ = extend_term_list($1, $3); }
                   | term 
                   { $$ = create_term_list($1); }
;
term:              NAME LPAREN terms RPAREN 
                   { $$ = create_function_term($1, $3); }
                   | NAME
                   { $$ = parser_create_constant_term(th, $1); }
                   | VARIABLE 
                   { $$ = create_variable($1, th); }
;
disjunction:            conjunction  { $$ = create_disjunction($1);}
                    | FALSE OR_SEPARATOR disjunction { $$ = $3; }
                     | disjunction OR_SEPARATOR FALSE { $$ = $1; }
                                 | disjunction OR_SEPARATOR conjunction { $$ = extend_disjunction($1, $3);}
;

strategy_def:         STRING_NEXT LPAREN prolog_arg_list RPAREN PERIOD { ; }
                      | STRING_ENABLED LPAREN prolog_arg_list RPAREN PERIOD { ; }
                      | STRING_ENABLED LPAREN prolog_arg_list RPAREN COLON DASH prolog_atom_list PERIOD { ; }
;

prolog_atom_list : prolog_atom COMMA prolog_atom_list { ; }
                   | prolog_atom {;}
;
prolog_atom :  NAME LPAREN RPAREN {;}
             | NAME LPAREN prolog_arg_list RPAREN {;}
             | prolog_arg prolog_infix_op prolog_arg {;}
;
prolog_infix_op: EQUALS {;}
               | NOT_EQUALS { ; }
               | TIMES {;}
               | SLASH {;}
prolog_arg_list:     prolog_arg COMMA prolog_arg_list {;}
                     | prolog_arg {;}
;
prolog_arg:         UNDERSCORE {;}
                          | VARIABLE {;}
                          | LSQPAREN prolog_list_content RSQPAREN { ; }
                          | LSQPAREN RSQPAREN { ; }
                          | NAME {;}
                          | NAME LPAREN prolog_arg_list RPAREN {;}
;
prolog_list_content : VARIABLE HOR_BAR VARIABLE {;}
                      | prolog_arg { ; }
                      | prolog_list_content COMMA prolog_arg {;}
%%

				   


void clpl_error(YYLTYPE *locp, char const *msg){
  fprintf(stderr, "Error in parsing file: %s",msg);
  fprintf(stderr, " line  %d\n", clpl_lineno);
  //fprintf(stderr, " line  %d\n", locp->last_line);
  exit(2);
}


theory* clpl_parser(FILE* f){
  th = create_theory();
  clpl_in = f;
  clpl_lineno = 1;
  //yydebug = 1;
  clpl_parse();
  finalize_theory(th);
  return th;
}
