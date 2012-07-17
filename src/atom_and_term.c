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
clp_atom* _common_create_atom(const term_list * args){
  clp_atom* ret_val =  ret_val = malloc_tester(sizeof(clp_atom));
  assert(args->n_args < args->size_args || (args->size_args == 0 && args->n_args == 0));
  ret_val->args = args;
  return ret_val;
}

clp_atom* parser_create_atom(const char* name, const term_list * args, theory* th){
  clp_atom* ret_val =  _common_create_atom(args);
  ret_val->pred = parser_new_predicate(th, name, args->n_args);
  return ret_val;
}


clp_atom* prover_create_atom(const clp_predicate* pred, const term_list * args){
  clp_atom* ret_val =  _common_create_atom(args);
  ret_val->pred = pred;
  return ret_val;
}

clp_atom* create_prop_variable(const char* name, theory* th){
  return parser_create_atom(name, _create_term_list(0), th);
}

void delete_atom(clp_atom* at){
  delete_term_list((term_list*) at->args);
  free(at);
}

/**
   Creating rete network from atoms
**/
rete_node* create_rete_atom_node(rete_net* net, const clp_atom* a, const freevars* vars, bool propagate, bool in_positive_lhs_part, unsigned int axiom_no){
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

clp_term* _init_term(enum term_type type, const term_list* args){
  clp_term* ret_val = malloc_tester(sizeof(clp_term));
  ret_val->type = type;
  ret_val->args = args;
  return ret_val;
}
clp_term* create_function_term(const char* function, const term_list *args){
  clp_term* t =  _init_term(function_term, args);
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
clp_term* parser_create_constant_term(theory* th, const char* name){
  clp_term* t =  _init_term(constant_term, _create_term_list(0));
  t->val.constant = parser_new_constant(th->constants, name);
  return t;
}

/**
   Called from constants.c to get a new term.
   The number is already ok.
**/
clp_term* prover_create_constant_term(dom_elem constant){
  clp_term* t = _init_term(constant_term, _create_term_list(0));
  t->val.constant = constant;
  return t;
}

clp_term * create_variable(const char* name, theory* th){
  freevars* fv = th->vars;
  clp_variable* var_name = parser_new_variable(&fv, name);
  clp_term * t =  _init_term(variable_term, _create_term_list(0));
  th->vars = fv;
  t->val.var = var_name;
  return t;
}

clp_term* copy_term(const clp_term* t){
  clp_term *c = _init_term(t->type, copy_term_list(t->args));
  c->val = t->val;
  return c;
}
    
void delete_term(clp_term* t){
  delete_term_list((term_list*) t->args);
  free(t);
}

bool test_ground_term(const clp_term* t, const constants* cs){
  assert(t != NULL);
  switch(t->type){
  case variable_term:
    fprintf(stderr, "Variable %s occurred in ground term\n", t->val.var->name);
    return false;
    break;
  case function_term:
    assert(test_term_list(t->args, cs));
  case constant_term:
    assert(test_constant(t->val.constant, cs));
    break;
  default:
    assert(false);
  }
  return true;
}

bool test_term(const clp_term* t, const constants* cs){
  assert(t != NULL);
  switch(t->type){
  case variable_term:
    assert(strlen(t->val.var->name) > 0);
    break;
  case function_term:
    assert(test_term_list(t->args, cs));
  case constant_term:
    assert(test_constant(t->val.constant, cs));
    break;
  default:
    assert(false);
  }
  return true;
}

bool test_term_list(const term_list* tl, const constants* cs){
  unsigned int i;
  assert(tl != NULL);
  assert(tl->n_args <= tl->size_args);
  for(i = 0; i < tl->n_args; i++)
    assert(test_term(tl->args[i], cs));
  return true;
}
    
bool test_ground_term_list(const term_list* tl, const constants* cs){
  unsigned int i;
  assert(tl != NULL);
  assert(tl->n_args <= tl->size_args);
  for(i = 0; i < tl->n_args; i++)
    assert(test_ground_term(tl->args[i], cs));
  return true;
}
    
bool test_atom(const clp_atom* a, const constants* cs){
  assert(test_predicate(a->pred));
  assert(test_term_list(a->args, cs));
  assert(a->pred->arity == a->args->n_args);
  return true;
}

    
bool test_ground_atom(const clp_atom* a, const constants* cs){
  assert(test_predicate(a->pred));
  assert(test_ground_term_list(a->args, cs));
  assert(a->pred->arity == a->args->n_args);
  return true;
}


term_list* _create_term_list(size_t size_args){
  term_list * ret_val = malloc_tester(sizeof(term_list));
  ret_val->n_args = 0;
  ret_val->size_args = size_args;
  ret_val->args = calloc_tester(size_args, sizeof(clp_term*));
  return ret_val;
}



clp_atom* parser_create_equality(const clp_term * t1, const clp_term * t2, theory* th){
  clp_atom* a;
  term_list* tl = _create_term_list(2);
  tl = extend_term_list(tl, t1);
  tl = extend_term_list(tl, t2);
  a = parser_create_atom("=", tl, th);
  return a;
}


/**
   term list constructors and destructors
**/
term_list* create_term_list(const clp_term *t){
  term_list * tl = _create_term_list(10); 
  return extend_term_list(tl, t);
}
term_list* extend_term_list(term_list* tl, const clp_term * t){
  assert(tl->n_args < tl->size_args || (tl->size_args == 0 && tl->n_args == 0));
  tl->n_args ++;
  if(tl->n_args >= tl->size_args){
    tl->size_args *= 2;
    tl->args = realloc_tester(tl->args, (tl->size_args+1) * sizeof(clp_term*) );
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
    delete_term((clp_term*) tl->args[i]);
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

freevars* free_atom_variables(const clp_atom *at, freevars* vars){
  return free_term_list_variables(at->args, vars);
}

freevars* free_term_variables(const clp_term *t, freevars* vars){
  if(t->type == variable_term){
    assert(t->args->n_args == 0);
    return add_freevars(vars, t->val.var);
  } else if (t->type == function_term) 
    return free_term_list_variables(t->args, vars);
  return vars;
}
bool _equal_terms(const clp_term* t1, const clp_term* t2, constants* constants, timestamps* ts, timestamp_store* store, bool update_ts, bool literal);
/**
   Comparators
   Return 0 for equality
**/
bool equal_term_lists(const term_list* t1, const term_list* t2, constants* constants, timestamps* ts, timestamp_store* store, bool update_ts, bool literal){
  unsigned int i;
  if(t1->n_args != t2->n_args)
    return false;
  for(i = 0; i < t1->n_args; i++){
    if(!_equal_terms(t1->args[i], t2->args[i], constants, ts, store, update_ts, literal))
      return false;
  }
  return true;
}

/**
   Returns true if the two terms are equal modulo equality of constants

   If update_ts is true, ts is updated (also if the terms are unequal.)

   If literal is true, the constants must have same name
**/
bool _equal_terms(const clp_term* t1, const clp_term* t2, constants* constants, timestamps* ts, timestamp_store* store, bool update_ts, bool literal){
  bool retval;
  if(t1->type != t2->type)
    return false;
  switch(t1->type){
  case variable_term:
    retval = (t1->val.var->var_no == t2->val.var->var_no);
    break;
  case constant_term:
    if(literal)
      retval = t1->val.constant.id == t2->val.constant.id;
    else
      retval = equal_constants_mt(t1->val.constant, t2->val.constant, constants, ts, store, update_ts);
    break;
  case function_term:
    retval = (t1->val.function == t2->val.function && equal_term_lists(t1->args, t2->args, constants, ts, store, update_ts, literal));
    break;
  default:
    fprintf(stderr, "Untreated term type in equal_terms in atom_and_term.c.\n");
    exit(EXIT_FAILURE);
  }
  return retval;
}

bool equal_terms(const clp_term* t1, const clp_term* t2, constants* constants, timestamps* ts, timestamp_store* store, bool update_ts){
  return _equal_terms(t1, t2, constants, ts, store, update_ts, false);
}
bool literally_equal_terms(const clp_term* t1, const clp_term* t2, constants* constants, timestamps* ts, timestamp_store* store, bool update_ts){
  return _equal_terms(t1, t2, constants, ts, store, update_ts, true);
}

/* 
   Assumes t is a constant or variable. 
   Returns the constant, or the value in sub of the constant
**/
dom_elem get_dom_elem(const clp_term* t, const substitution* sub){
  assert(t->type != function_term);
  if(t->type == constant_term)
    return t->val.constant;
  t = get_sub_value(sub, t->val.var->var_no);
  assert(t->type == constant_term);
  return t->val.constant;
}

/**
   Used to check to terms of an equality.
**/
bool true_ground_equality(const clp_term* t1, const clp_term* t2, const substitution* sub, constants* cs, timestamps* ts, timestamp_store* store, bool update_ts){
  dom_elem c1, c2;
  c1 = get_dom_elem(t1, sub);
  c2 = get_dom_elem(t2, sub);
  return equal_constants_mt(c1, c2, cs, ts, store, update_ts);
}

bool equal_atoms(const clp_atom* a1, const clp_atom* a2, constants* constants, timestamps* ts, timestamp_store* store, bool update_ts){
  if(a1->pred->pred_no != a2->pred->pred_no)
    return false;
  return equal_term_lists(a1->args, a2->args, constants, ts, store, update_ts, false);
}


/**
   Generic printing facilities
**/
void print_term_list(const term_list *tl, const constants* cs, FILE* stream, char* separator, bool parentheses, void (*print_term)(const clp_term*, const constants*, FILE*)){
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
void print_atom(const clp_atom *at, const constants* cs, FILE* stream, void (*print_term_list)(const term_list*, const constants*, FILE*)){
  fprintf(stream, "%s", at->pred->name);
  print_term_list(at->args, cs, stream);
}

/**
   Prints the term. Integer constants are prefixed with num_ because of a problem with integers in coq
**/
void print_term(const clp_term *t, const constants* cs, FILE* stream, void (*print_term_list)(const term_list*, const constants*, FILE*)){
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
void print_fol_atom(const clp_atom *at, const constants* cs, FILE* stream){
  print_atom(at, cs, stream, print_fol_term_list);
}

void print_fol_term_list(const term_list *tl, const constants* cs, FILE* stream){
  print_term_list(tl, cs, stream, ",", true, print_fol_term);
}

void print_fol_term(const clp_term *t, const constants* cs, FILE* s){
  print_term(t, cs, s, print_fol_term_list);
}
/**
   Printing geolog input format
**/
void print_geolog_atom(const clp_atom *at, const constants* cs, FILE* stream){
  print_atom(at, cs, stream, print_fol_term_list);
}

void print_geolog_term_list(const term_list *tl, const constants* cs, FILE* stream){
  print_term_list(tl, cs, stream, ",", true, print_fol_term);
}

void print_geolog_term(const clp_term *t, const constants* cs, FILE* s){
  print_term(t, cs, s, print_fol_term_list);
}

/**
   coq output
**/
void print_coq_term_list(const term_list *tl, const constants* cs, FILE* stream){
  print_term_list(tl, cs, stream, " ", false, print_coq_term);
}

void print_coq_term(const clp_term *t, const constants* cs, FILE* stream){
  print_term(t, cs, stream, print_coq_term_list);
}
void print_coq_atom(const clp_atom* a, const constants* cs, FILE* stream){
  if(a->pred->is_equality){
    print_coq_term(a->args->args[0], cs, stream);
    fprintf(stream, " = ");
    print_coq_term(a->args->args[1], cs, stream);
  } else {
    fprintf(stream, "%s ", a->pred->name);
    print_coq_term_list(a->args, cs, stream);
  }
}

