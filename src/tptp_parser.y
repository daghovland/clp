/* tptp_parser.y

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
   Parses the format used by TPTP

   thf (typed higher order form) is not supported, since an efficient translation to cl is not known to me

   See http://www.cs.miami.edu/~tptp/TPTP/SyntaxBNF.html
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

 

  int tptp_lex();
  
  theory *th;

  extern FILE* tptp_in;
  
  extern int fileno(FILE*);
  extern int tptp_lineno;

%}

%define api.pure
%name-prefix "tptp_"
%locations
%error-verbose
%glr-parser


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

%token comment
%token zero_numeric
%token non_zero_numeric
%token <int> numeric
%token <chr> lower_alpha
%token <chr> upper_alpha
%token <str> lower_word
%token <str> upper_word
%token <str> dollar_word
%token <str> dollar_dollar_word
%token alpha_numeric
%token printable_char
%token PERIOD
%token COMMA
%token COLON
%token LPAREN
%token RPAREN
%token LSQPAREN
%token RSQPAREN
%token VLINE
%token AMPERSAND
%token ATSIGN
%token STAR
%token PLUS
%token MINUS
%token TILDE
%token HAT
%token EQUALS
%token DOLLAR
%token ARROW
%token LESS_SIGN
%token PERCENTAGE_SIGN
%token EXCLAMATION
%token QUESTION
%token thf_string
%token tff_string
%token fof_string
%token cnf_string
%token include_string
%token true_string
%token false_string
%token integer
%token rational
%token real
%token distinct_object
%token single_quoted

%start TPTP_file

%{
  void tptp_error(YYLTYPE*, char const*);
  %}

%%
fof_annotated      : fof_string LPAREN name COMMA formula_role COMMA fof_formula RPAREN PERIOD
;
cnf_annotated      : cnf_string LPAREN name COMMA formula_role COMMA cnf_formula RPAREN PERIOD
;
formula_role       : lower_word
;
fof_formula        : fof_logic_formula | fof_sequent
;
fof_logic_formula  : fof_binary_formula | fof_unitary_formula
;
fof_binary_formula : fof_binary_nonassoc | fof_binary_assoc
;
fof_binary_nonassoc : fof_unitary_formula binary_connective fof_unitary_formula
;
fof_binary_assoc   : fof_or_formula | fof_and_formula
;
fof_or_formula     : fof_unitary_formula VLINE fof_unitary_formula |
                         fof_or_formula VLINE fof_unitary_formula
;
fof_and_formula    : fof_unitary_formula  AMPERSAND  fof_unitary_formula |
                         fof_and_formula  AMPERSAND  fof_unitary_formula
;
fof_unitary_formula : fof_quantified_formula | fof_unary_formula |
                         atomic_formula |  LPAREN fof_logic_formula RPAREN 
;
fof_quantified_formula : fol_quantifier LSQPAREN fof_variable_list RSQPAREN  COLON
                         fof_unitary_formula
;
fof_variable_list  : variable | variable COMMA fof_variable_list
;
fof_unary_formula  : unary_connective fof_unitary_formula | fol_infix_unary
;
fof_sequent        : fof_tuple gentzen_arrow fof_tuple | LPAREN fof_sequent RPAREN 
;
fof_tuple          :  LSQPAREN  RSQPAREN  |  LSQPAREN fof_tuple_list RSQPAREN 
;
fof_tuple_list     : fof_logic_formula | fof_logic_formula COMMA fof_tuple_list
;
cnf_formula        :  LPAREN disjunction RPAREN  | disjunction
;
disjunction        : literal | disjunction VLINE literal
;
literal            : atomic_formula |  TILDE  atomic_formula |
                         fol_infix_unary
;
fol_infix_unary    : term infix_inequality term
;
fol_quantifier     :  EXCLAMATION  |  QUESTION 
;
binary_connective  : LESS_SIGN EQUALS ARROW | EQUALS ARROW | LESS_SIGN EQUALS |  LESS_SIGN TILDE ARROW  |  TILDE VLINE |  TILDE  AMPERSAND 
;
unary_connective   :  TILDE 
;
gentzen_arrow      : MINUS MINUS ARROW
;
defined_type       : atomic_defined_word
;
atomic_formula     : plain_atomic_formula | defined_atomic_formula | system_atomic_formula
;
plain_atomic_formula : plain_term
;
plain_atomic_formula : proposition | predicate LPAREN arguments RPAREN 
;
proposition        : predicate
;
predicate          : atomic_word
;
defined_atomic_formula : defined_plain_formula | defined_infix_formula
;
defined_plain_formula : defined_plain_term
;
defined_plain_formula : defined_prop | defined_pred LPAREN arguments RPAREN 
;
defined_prop       : atomic_defined_word
;
defined_prop       :  DOLLAR true_string |  DOLLAR false_string
;
defined_pred       : atomic_defined_word
;
defined_infix_formula  : term defined_infix_pred term
;
defined_infix_pred : infix_equality
;
infix_equality     : EQUALS
;
infix_inequality   :  EXCLAMATION EQUALS
;
system_atomic_formula : system_term
;
term               : function_term | variable
;
function_term      : plain_term | defined_term | system_term
;
plain_term         : constant | functor LPAREN arguments RPAREN 
;
constant           : functor
;
functor            : atomic_word
;
defined_term       : defined_atom | defined_atomic_term
;
defined_atom       : number | distinct_object
;
defined_atomic_term : defined_plain_term
;
defined_plain_term : defined_constant | defined_functor LPAREN arguments RPAREN 
;
defined_constant   : defined_functor
;
defined_constant   : defined_type
;
defined_functor    : atomic_defined_word
;
system_term        : system_constant | system_functor LPAREN arguments RPAREN 
;
system_constant    : system_functor
;
system_functor     : atomic_system_word
;
variable           : upper_word
;
arguments          : term | term COMMA arguments
;
name               : atomic_word | integer
;
atomic_word        : lower_word | single_quoted
		       ;
atomic_defined_word : dollar_word
		       ;
atomic_system_word : dollar_dollar_word
;
number             : integer | rational | real
;
annotated_formula  :  fof_annotated {;} | cnf_annotated {;}
;

TPTP_input : annotated_formula {;}

TPTP_file : TPTP_input TPTP_file {;}
           | TPTP_input {;}
%%

				   


void tptp_error(YYLTYPE *locp, char const *msg){
  fprintf(stderr, "Error in parsing file: %s",msg);
  fprintf(stderr, " line  %d\n", tptp_lineno);
  //fprintf(stderr, " line  %d\n", locp->last_line);
  exit(2);
}


theory* tptp_parser(FILE* f){
  th = create_theory();
  tptp_in = f;
  tptp_lineno = 1;
  //yydebug = 1;
  tptp_parse();
  finalize_theory(th);
  return th;
}
