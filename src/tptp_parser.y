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
%expect-rr 1
%expect 1


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

%token comment
%token zero_numeric
%token non_zero_numeric
%token <int> numeric
%token <chr> lower_alpha
%token <chr> upper_alpha
%token <str> lower_word
%token <str> upper_word
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



%start TPTP_file

%{
  void tptp_error(YYLTYPE*, char const*);
  %}

%%
null               :
		       ;

tff_annotated      : tff_string LPAREN name COMMA formula_role COMMA tff_formula annotations RPAREN PERIOD
;
fof_annotated      : fof_string LPAREN name COMMA formula_role COMMA fof_formula annotations RPAREN PERIOD
;
cnf_annotated      : cnf_string LPAREN name COMMA formula_role COMMA cnf_formula annotations RPAREN PERIOD
;
annotations        : COMMA source optional_info | null
;

formula_role       : lower_word
;
thf_formula        : thf_logic_formula | thf_sequent
   ;

tff_formula        : tff_logic_formula | tff_typed_atom | tff_sequent
;
tff_logic_formula  : tff_binary_formula | tff_unitary_formula
;
tff_binary_formula : tff_binary_nonassoc | tff_binary_assoc
;
tff_binary_nonassoc : tff_unitary_formula binary_connective
                         tff_unitary_formula
;
tff_binary_assoc   : tff_or_formula | tff_and_formula
;
tff_or_formula     : tff_unitary_formula VLINE tff_unitary_formula |
                         tff_or_formula VLINE tff_unitary_formula
;
tff_and_formula    : tff_unitary_formula  AMPERSAND  tff_unitary_formula |
                         tff_and_formula  AMPERSAND  tff_unitary_formula
;
tff_unitary_formula : tff_quantified_formula | tff_unary_formula |
                         atomic_formula | tff_conditional | tff_let |
                          LPAREN tff_logic_formula RPAREN 
;
tff_quantified_formula : fol_quantifier  LSQPAREN tff_variable_list RSQPAREN  COLON
                         tff_unitary_formula
			 ;
tff_variable_list  : tff_variable | tff_variable COMMA tff_variable_list
;
tff_variable       : tff_typed_variable | variable
;
tff_typed_variable : variable COLON tff_atomic_type
;
tff_unary_formula  : unary_connective tff_unitary_formula |
                         fol_infix_unary
;
tff_conditional    :  DOLLAR ite_f LPAREN tff_logic_formula COMMA tff_logic_formula COMMA 
                         tff_logic_formula RPAREN 
;
tff_let            :  DOLLAR let_tf LPAREN tff_let_term_defn COMMA tff_formula RPAREN  |
                          DOLLAR let_ff LPAREN tff_let_formula_defn COMMA tff_formula RPAREN 
;
tff_let_term_defn  :  EXCLAMATION   LSQPAREN tff_variable_list RSQPAREN  COLON tff_let_term_binding |
                         tff_let_term_binding
;
tff_let_term_binding : term EQUALS term
;
tff_let_formula_defn :  EXCLAMATION   LSQPAREN tff_variable_list RSQPAREN  COLON tff_let_formula_binding |
                         tff_let_formula_binding
tff_let_formula_binding : atomic_formula EQUALS tff_unitary_formula

tff_sequent        : tff_tuple gentzen_arrow tff_tuple |
                          LPAREN tff_sequent RPAREN 
tff_tuple          :  LSQPAREN  RSQPAREN  |  LSQPAREN tff_tuple_list RSQPAREN 
			   ;
tff_tuple_list     : tff_logic_formula |
                         tff_logic_formula COMMA tff_tuple_list
;

tff_typed_atom     : tff_untyped_atom COLON tff_top_level_type |
                          LPAREN tff_typed_atom RPAREN 
;
tff_untyped_atom   : functor | system_functor
;


tff_top_level_type : tff_atomic_type | tff_mapping_type |
                         tff_quantified_type
;
tff_quantified_type :  EXCLAMATION   LSQPAREN tff_variable_list RSQPAREN  COLON tff_monotype |
                           LPAREN tff_quantified_type RPAREN 
;
tff_monotype       : tff_atomic_type |  LPAREN tff_mapping_type RPAREN 
;
tff_unitary_type   : tff_atomic_type |  LPAREN tff_xprod_type RPAREN 
;
tff_atomic_type    : atomic_word | defined_type | 
                         atomic_word LPAREN tff_type_arguments RPAREN  | variable
;
tff_type_arguments : tff_atomic_type |
                         tff_atomic_type COMMA tff_type_arguments
;
tff_mapping_type   : tff_unitary_type ARROW tff_atomic_type |
                          LPAREN tff_mapping_type RPAREN 
;
tff_xprod_type     : tff_atomic_type STAR tff_atomic_type |
                         tff_xprod_type STAR tff_atomic_type |
                          LPAREN tff_xprod_type RPAREN 
;
fof_formula        : fof_logic_formula | fof_sequent
;
fof_logic_formula  : fof_binary_formula | fof_unitary_formula
;
fof_binary_formula : fof_binary_nonassoc | fof_binary_assoc
;
fof_binary_nonassoc : fof_unitary_formula binary_connective
                         fof_unitary_formula
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
fof_quantified_formula : fol_quantifier  LSQPAREN fof_variable_list RSQPAREN  COLON
                         fof_unitary_formula
;
fof_variable_list  : variable | variable COMMA fof_variable_list
;
fof_unary_formula  : unary_connective fof_unitary_formula |
                         fol_infix_unary
;
fof_sequent        : fof_tuple gentzen_arrow fof_tuple |
                          LPAREN fof_sequent RPAREN 
;
fof_tuple          :  LSQPAREN  RSQPAREN  |  LSQPAREN fof_tuple_list RSQPAREN 
;
fof_tuple_list     : fof_logic_formula |
                         fof_logic_formula COMMA fof_tuple_list
;


cnf_formula        :  LPAREN disjunction RPAREN  | disjunction
;
disjunction        : literal | disjunction VLINE literal
;
literal            : atomic_formula |  TILDE  atomic_formula |
                         fol_infix_unary
;
thf_conn_term      : thf_pair_connective | assoc_connective |
                         thf_unary_connective
;
fol_infix_unary    : term infix_inequality term
;

thf_quantifier     : fol_quantifier | HAT |  EXCLAMATION  |  QUESTION  STAR  |  ATSIGN PLUS |  ATSIGN MINUS
;
thf_pair_connective : infix_equality | infix_inequality |
                         binary_connective
;
thf_unary_connective : unary_connective |  EXCLAMATION  EXCLAMATION  |  QUESTION  QUESTION 
;
subtype_sign       : less_signless_sign
;
fol_quantifier     :  EXCLAMATION  |  QUESTION 
;
binary_connective  : LESS_SIGN EQUALS ARROW | EQUALS ARROW | LESS_SING EQUALS |  LESS_SIGN TILDE ARROW  |  TILDE VLINE |  TILDE  AMPERSAND 
;
assoc_connective   : VLINE |  AMPERSAND 
;
unary_connective   :  TILDE 
;
gentzen_arrow      : MINUS MINUS ARROW
;

defined_type       : atomic_defined_word
;
defined_type       :  DOLLAR oType |  DOLLAR o |  DOLLAR iType |  DOLLAR i |  DOLLAR tType |
                          DOLLAR real |  DOLLAR rat |  DOLLAR int
;
system_type        : atomic_system_word
;


atomic_formula     : plain_atomic_formula | defined_atomic_formula |
                         system_atomic_formula
plain_atomic_formula : plain_term
plain_atomic_formula : proposition | predicate LPAREN arguments RPAREN 
proposition        : predicate
predicate          : atomic_word

defined_atomic_formula : defined_plain_formula | defined_infix_formula
defined_plain_formula : defined_plain_term
defined_plain_formula : defined_prop | defined_pred LPAREN arguments RPAREN 
defined_prop       : atomic_defined_word
defined_prop       :  DOLLAR true |  DOLLAR false

defined_pred       : atomic_defined_word
		     /*defined_pred       :  DOLLAR distinct |
                          DOLLAR less |  DOLLAR lesseq |  DOLLAR greater |  DOLLAR greatereq |
                          DOLLAR is_int |  DOLLAR is_rat
		     */
defined_infix_formula  : term defined_infix_pred term
defined_infix_pred : infix_equality
infix_equality     : EQUALS
infix_inequality   :  EXCLAMATION EQUALS

system_atomic_formula : system_term

term               : function_term | variable | conditional_term |
                         let_term
function_term      : plain_term | defined_term | system_term
plain_term         : constant | functor LPAREN arguments RPAREN 
constant           : functor
functor            : atomic_word

defined_term       : defined_atom | defined_atomic_term
defined_atom       : number | distinct_object
defined_atomic_term : defined_plain_term

defined_plain_term : defined_constant | defined_functor LPAREN arguments RPAREN 
defined_constant   : defined_functor
defined_constant   : defined_type
defined_functor    : atomic_defined_word


system_term        : system_constant | system_functor LPAREN arguments RPAREN 
system_constant    : system_functor
system_functor     : atomic_system_word

variable           : upper_word

arguments          : term | term COMMA arguments

principal_symbol   : functor | variable

conditional_term   :  DOLLAR ite_t LPAREN tff_logic_formula COMMA term COMMA term RPAREN 

let_term           :  DOLLAR let_ft LPAREN tff_let_formula_defn COMMA term RPAREN  |
                          DOLLAR let_tt LPAREN tff_let_term_defn COMMA term RPAREN 


source             : general_term
source             : dag_source | internal_source | external_source |
                         unknown |  LSQPAREN sources RSQPAREN 
sources            : source | source COMMA sources


dag_source         : name | inference_record
inference_record   : inference LPAREN inference_rule COMMA useful_info COMMA 
                              LSQPAREN parent_list RSQPAREN  RPAREN 
inference_rule     : atomic_word

parent_list        : parent_info | parent_info COMMA parent_list
parent_info        : source parent_details
parent_details     : COLON general_list | null
internal_source    : introduced LPAREN intro_type optional_info RPAREN 
intro_type         : definition | axiom_of_choice | tautology | assumption

external_source    : file_source | theory | creator_source
file_source        : file LPAREN file_name file_info RPAREN 
file_info          :  COMMA name | null
theory             : theory LPAREN theory_name optional_info RPAREN 
theory_name        : equality | ac

creator_source     : creator LPAREN creator_name optional_info RPAREN 
creator_name       : atomic_word


optional_info      :  COMMA useful_info | null
useful_info        : general_list
useful_info        :  LSQPAREN  RSQPAREN  |  LSQPAREN info_items RSQPAREN 
info_items         : info_item | info_item COMMA info_items
info_item          : formula_item | inference_item | general_function

formula_item       : description_item | iquote_item
description_item   : description LPAREN atomic_word RPAREN 
iquote_item        : iquote LPAREN atomic_word RPAREN 

inference_item     : inference_status | assumptions_record |
                         new_symbol_record | refutation
inference_status   : status LPAREN status_value RPAREN  | inference_info
;
status_value       : lower_word
;
inference_info     : inference_rule LPAREN atomic_word COMMA general_list RPAREN 

assumptions_record : assumptions LPAREN  LSQPAREN name_list RSQPAREN  RPAREN 

refutation         : refutation LPAREN file_source RPAREN 

new_symbol_record  : new_symbols LPAREN atomic_word COMMA  LSQPAREN new_symbol_list RSQPAREN  RPAREN 
new_symbol_list    : principal_symbol |
                         principal_symbol COMMA new_symbol_list



general_term       : general_data | general_data:general_term |
                         general_list
		       ;
general_data       : atomic_word | general_function |
                         variable | number | distinct_object |
                         formula_data
general_function   : atomic_word LPAREN general_terms RPAREN
		       ;

general_data       : bind LPAREN variable COMMA formula_data RPAREN 
formula_data       :  DOLLAR thf_string LPAREN thf_formula RPAREN  |  DOLLAR tff_string LPAREN tff_formula RPAREN  |
                          DOLLAR fof_string LPAREN fof_formula RPAREN  |  DOLLAR cnf_string LPAREN cnf_formula RPAREN  |
                          DOLLAR fot LPAREN term RPAREN 
;
general_list       : LSQPAREN RSQPAREN | LSQPAREN general_terms RSQPAREN
		       ;
general_terms      : general_term | general_term COMMA general_terms
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
annotated_formula  :  tff_annotated {;} | fof_annotated {;} | cnf_annotated {;}
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
