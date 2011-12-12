/* atom_and_term.c

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

#include "common.h"
#include "term.h"
#include "atom.h"
#include "fresh_constants.h"
#include "rete.h"
#include "predicate.h"
#include "constants.h"

/**
   Functions used by the geolog_parser to create the rules data structures 

**/
term_list* _create_term_list(size_t);


/**
   atom constructors and destructors
**/
atom* _common_create_atom(const term_list * args){
  atom* ret_val =  ret_val = malloc_tester(sizeof(atom));
  assert(args->n_args < args->size_args || (args->size_args == 0 && args->n_args == 0));
  ret_val->args = args;
  return ret_val;
}

atom* parser_create_atom(const char* name, const term_list * args, theory* th){
  atom* ret_val =  _common_create_atom(args);
  ret_val->pred = parser_new_predicate(th, name, args->n_args);
  return ret_val;
}


atom* prover_create_atom(const predicate* pred, const term_list * args){
  atom* ret_val =  _common_create_atom(args);
  ret_val->pred = pred;
  return ret_val;
}



atom* create_prop_variable(const char* name, theory* th){
  return parser_create_atom(name, _create_term_list(0), th);
}

void delete_atom(atom* at){
  delete_term_list((term_list*) at->args);
  free(at);
}

/**
   Creating rete network from atoms
**/
rete_node* create_rete_atom_node(rete_net* net, const atom* a, const freevars* vars, bool propagate, bool in_positive_lhs_part, size_t axiom_no){
  unsigned int i;
  rete_node *p;
  p = get_selector(a->pred->pred_no, net);
  assert(p->size_children > 0);
  for(i = 0; i < a->args->n_args; i++){
    rete_node* child = create_alpha_node(p, i, a->args->args[i], vars, true, in_positive_lhs_part, axiom_no);
    assert(child->size_children > 0);
    p = child;
  }
  p->val.alpha.propagate = propagate;
  return p;
}


/**
   term constructors and destructors
**/

term* _init_term(enum term_type type, const term_list* args){
  term* ret_val = malloc_tester(sizeof(term));
  ret_val->type = type;
  ret_val->args = args;
  return ret_val;
}
term* create_function_term(const char* function, const term_list *args){
  term* t =  _init_term(function_term, args);
  t->val.function = function;
  return t;
}

/**
   Since equality of constants is later checked based on
   pointer equality of "char* name", we use parser_new_constant
   to check if this constant name has already been used

   In the prover, fresh_exist_constant creates always new constants, 
   so we do not need to check this
**/
term* parser_create_constant_term(theory* th, const char* name){
  term* t =  _init_term(constant_term, _create_term_list(0));
  t->val.constant = parser_new_constant(th->constants, name);
  return t;
}

/**
   Called from constants.c to get a new term.
   The number is already ok.
**/
term* prover_create_constant_term(unsigned int constant){
  term* t = _init_term(constant_term, _create_term_list(0));
  t->val.constant = constant;
  return t;
}

term * create_variable(const char* name, theory* th){
  freevars* fv = th->vars;
  variable* var_name = parser_new_variable(&fv, name);
  term * t =  _init_term(variable_term, _create_term_list(0));
  th->vars = fv;
  t->val.var = var_name;
  return t;
}

term* copy_term(const term* t){
  term *c = _init_term(t->type, copy_term_list(t->args));
  c->val = t->val;
  return c;
}
    
void delete_term(term* t){
  delete_term_list((term_list*) t->args);
  free(t);
}

bool test_ground_term(const term* t){
  assert(t != NULL);
  switch(t->type){
  case variable_term:
    fprintf(stderr, "Variable %s occurred in ground term\n", t->val.var->name);
    return false;
    break;
  case function_term:
    assert(test_term_list(t->args));
  case constant_term:
    break;
  default:
    assert(false);
  }
  return true;
}

bool test_term(const term* t){
  assert(t != NULL);
  switch(t->type){
  case variable_term:
    assert(strlen(t->val.var->name) > 0);
    break;
  case function_term:
    assert(test_term_list(t->args));
  case constant_term:
    break;
  default:
    assert(false);
  }
  return true;
}

bool test_term_list(const term_list* tl){
  unsigned int i;
  assert(tl != NULL);
  assert(tl->n_args <= tl->size_args);
  for(i = 0; i < tl->n_args; i++)
    assert(test_term(tl->args[i]));
  return true;
}
    
bool test_ground_term_list(const term_list* tl){
  unsigned int i;
  assert(tl != NULL);
  assert(tl->n_args <= tl->size_args);
  for(i = 0; i < tl->n_args; i++)
    assert(test_ground_term(tl->args[i]));
  return true;
}
    
bool test_atom(const atom* a){
  assert(test_predicate(a->pred));
  assert(test_term_list(a->args));
  assert(a->pred->arity == a->args->n_args);
  return true;
}

    
bool test_ground_atom(const atom* a){
  assert(test_predicate(a->pred));
  assert(test_ground_term_list(a->args));
  assert(a->pred->arity == a->args->n_args);
  return true;
}


term_list* _create_term_list(size_t size_args){
  term_list * ret_val = malloc_tester(sizeof(term_list));
  ret_val->n_args = 0;
  ret_val->size_args = size_args;
  ret_val->args = calloc_tester(size_args, sizeof(term*));
  return ret_val;
}



atom* parser_create_equality(const term * t1, const term * t2, theory* th){
  atom* a;
  term_list* tl = _create_term_list(2);
  tl = extend_term_list(tl, t1);
  tl = extend_term_list(tl, t2);
  a = parser_create_atom("=", tl, th);
  return a;
}


/**
   term list constructors and destructors
**/
term_list* create_term_list(const term *t){
  term_list * tl = _create_term_list(10); 
  return extend_term_list(tl, t);
}
term_list* extend_term_list(term_list* tl, const term * t){
  assert(tl->n_args < tl->size_args || (tl->size_args == 0 && tl->n_args == 0));
  tl->n_args ++;
  if(tl->n_args >= tl->size_args){
    tl->size_args *= 2;
    tl->args = realloc_tester(tl->args, (tl->size_args+1) * sizeof(term*) );
  }
  tl->args[tl->n_args-1] = t;
  return tl;
}

term_list* copy_term_list(const term_list* tl){
  unsigned int i;
  term_list* c = _create_term_list(tl->n_args);
  for(i = 0; i < tl->n_args; i++)
    c->args[i] = copy_term(tl->args[i]);
  return c;
}

void delete_term_list(term_list* tl){
  int i;
  for(i = 0; i < tl->n_args; i++)
    delete_term((term*) tl->args[i]);
  free(tl->args);
  free(tl);
}



/**
   The free variables are returned, and a list of its strings is given in the second argument
**/
freevars* free_term_list_variables(const term_list * tl, freevars* vars){
  int i;
  assert(tl->n_args < tl->size_args || (tl->size_args == 0 && tl->n_args == 0));
  for(i = 0; i < tl->n_args; i++)
    vars = free_term_variables(tl->args[i], vars);
  return vars;
}

freevars* free_atom_variables(const atom *at, freevars* vars){
  return free_term_list_variables(at->args, vars);
}

freevars* free_term_variables(const term *t, freevars* vars){
  if(t->type == variable_term){
    assert(t->args->n_args == 0);
    return add_freevars(vars, t->val.var);
  } else if (t->type == function_term) 
    return free_term_list_variables(t->args, vars);
  return vars;
}
/**
   Comparators
   Return 0 for equality
**/
bool equal_term_lists(const term_list* t1, const term_list* t2, constants* constants){
  unsigned int i;
  if(t1->n_args != t2->n_args)
    return false;
  for(i = 0; i < t1->n_args; i++){
    if(!equal_terms(t1->args[i], t2->args[i], constants))
      return false;
  }
  return true;
}

/**
   Returns true if the two terms are equal modulo equality of constants
**/
bool equal_terms(const term* t1, const term* t2, constants* constants){
  bool retval;
  if(t1->type != t2->type)
    return false;
  switch(t1->type){
  case variable_term:
    retval = (t1->val.var->var_no == t2->val.var->var_no);
    break;
  case constant_term:
    retval = equal_constants(t1->val.constant, t2->val.constant, constants);
    break;
  case function_term:
    retval = (t1->val.function == t2->val.function && equal_term_lists(t1->args, t2->args, constants));
    break;
  default:
    fprintf(stderr, "Untreated term type in equal_terms in atom_and_term.c.\n");
    exit(EXIT_FAILURE);
  }
  return retval;
}

bool equal_atoms(const atom* a1, const atom* a2, constants* constants){
  if(a1->pred->pred_no != a2->pred->pred_no)
    return false;
  return equal_term_lists(a1->args, a2->args, constants);
}


/**
   Generic printing facilities
**/
void print_term_list(const term_list *tl, const constants* cs, FILE* stream, char* separator, bool parentheses, void (*print_term)(const term*, const constants*, FILE*)){
  int i;
  if(tl->n_args > 0){
    if(parentheses)
      fprintf(stream, "(");
    for(i = 0; i+1 < tl->n_args; i++){
      print_fol_term(tl->args[i], cs, stream);
      fprintf(stream, "%s", separator);
    }
    print_term(tl->args[i], cs, stream);
    if(parentheses)
      fprintf(stream, ")");
  }
}
void print_atom(const atom *at, const constants* cs, FILE* stream, void (*print_term_list)(const term_list*, const constants*, FILE*)){
  fprintf(stream, "%s", at->pred->name);
  print_term_list(at->args, cs, stream);
}

/**
   Prints the term. Integer constants are prefixed with num_ because of a problem with integers in coq
**/
void print_term(const term *t, const constants* cs, FILE* stream, void (*print_term_list)(const term_list*, const constants*, FILE*)){
  const char* name;
  char first; 
  switch(t->type){
  case variable_term:
    fprintf(stream, "%s", t->val.var->name);
    break;
  case constant_term:
    name = get_constant_name(t->val.constant, cs);
    first = name[0];
    if(first - '0' >= 0 && first - '0' <= 9)
      fprintf(stream, "num_%s", name);
    else
      fprintf(stream, "%s", name);
    break;
  case function_term:
    fprintf(stream, "%s", t->val.function);
    print_term_list(t->args, cs, stream);
    break;
  default:
    fprintf(stderr, "Untreated term type in atom_and_term.c: print_term.\n");
    exit(EXIT_FAILURE);
  }
}

/**
   Pretty printing standard fol format
**/
void print_fol_atom(const atom *at, const constants* cs, FILE* stream){
  print_atom(at, cs, stream, print_fol_term_list);
}

void print_fol_term_list(const term_list *tl, const constants* cs, FILE* stream){
  print_term_list(tl, cs, stream, ",", true, print_fol_term);
}

void print_fol_term(const term *t, const constants* cs, FILE* s){
  print_term(t, cs, s, print_fol_term_list);
}
/**
   Printing geolog input format
**/
void print_geolog_atom(const atom *at, const constants* cs, FILE* stream){
  print_atom(at, cs, stream, print_fol_term_list);
}

void print_geolog_term_list(const term_list *tl, const constants* cs, FILE* stream){
  print_term_list(tl, cs, stream, ",", true, print_fol_term);
}

void print_geolog_term(const term *t, const constants* cs, FILE* s){
  print_term(t, cs, s, print_fol_term_list);
}

/**
   coq output
**/
void print_coq_term_list(const term_list *tl, const constants* cs, FILE* stream){
  print_term_list(tl, cs, stream, " ", false, print_coq_term);
}

void print_coq_term(const term *t, const constants* cs, FILE* stream){
  print_term(t, cs, stream, print_coq_term_list);
}
void print_coq_atom(const atom* a, const constants* cs, FILE* stream){
  fprintf(stream, "%s ", a->pred->name);
  print_coq_term_list(a->args, cs, stream);
}
