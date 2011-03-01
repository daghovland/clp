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

/**
   Used as a key when searching
   a list of substitutions
**/
atom* create_bogus_atom(const char* name, size_t n_args){
  
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
rete_node* create_rete_atom_node(rete_net* net, const atom* a, const freevars* vars){
  unsigned int i;
  rete_node *p;
  p = get_selector(a->pred->pred_no, net);
  assert(p->size_children > 0);
  for(i = 0; i < a->args->n_args; i++){
    rete_node* child = create_alpha_node(p, i, a->args->args[i], vars);
    assert(child->size_children > 0);
    p = child;
  }
  return p;
}


/**
   term constructors and destructors
**/

term* _create_term(const char* name, variable* var, enum term_type type, const term_list* args){
  term* ret_val = malloc_tester(sizeof(term));
  assert( ( name != NULL && type != variable_term )
	  || ( var != NULL && type == variable_term ) );
  ret_val->name = name;
  ret_val->var = var;
  ret_val->type = type;
  ret_val->args = args;
  return ret_val;
}
term* create_term(const char* name, const term_list *args){
  return _create_term(name, NULL, function_term, args);
}

/**
   Since equality of constants is later checked based on
   pointer equality of "char* name", we use parser_new_constant
   to check if this constant name has already been used

   In the prover, fresh_exist_constant creates always new constants, 
   so we do not need to check this
**/
term* parser_create_constant_term(theory* th, const char* name){
  const char* cname = parser_new_constant(th, name);
  return _create_term(cname, NULL, constant_term, _create_term_list(0));
}

term* prover_create_constant_term(const char* name){
  return _create_term(name, NULL, constant_term, _create_term_list(0));
}

term * create_variable(const char* name, theory* th){
  variable* var_name = parser_new_variable(th->vars, name);
  return _create_term(name, var_name, variable_term, _create_term_list(0));
}

term* copy_term(const term* t){
  return _create_term(t->name, t->var, t->type, copy_term_list(t->args));
}
    
void delete_term(term* t){
  delete_term_list((term_list*) t->args);
  free(t);
}

bool test_ground_term(const term* t){
  assert(t != NULL);
  switch(t->type){
  case variable_term:
    fprintf(stderr, "Variable %s occurred in ground term\n", t->var->name);
    return false;
    break;
  case function_term:
    assert(test_term_list(t->args));
  case constant_term:
    assert(strlen(t->name) > 0);
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
    assert(strlen(t->var->name) > 0);
    break;
  case function_term:
    assert(test_term_list(t->args));
  case constant_term:
    assert(strlen(t->name) > 0);
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
void free_term_list_variables(const term_list * tl, freevars* vars){
  int i;
  assert(tl->n_args < tl->size_args || (tl->size_args == 0 && tl->n_args == 0));
  for(i = 0; i < tl->n_args; i++)
    free_term_variables(tl->args[i], vars);
  return;
}

void free_atom_variables(const atom *at, freevars* vars){
  return free_term_list_variables(at->args, vars);
}

void free_term_variables(const term *t, freevars* vars){
  if(t->type == variable_term){
    assert(t->args->n_args == 0);
    add_freevars(vars, t->var);
  } else if (t->type == function_term) 
    free_term_list_variables(t->args, vars);
  return;
}
/**
   Comparators
   Return 0 for equality
**/
bool equal_term_lists(const term_list* t1, const term_list* t2){
  unsigned int i;
  bool retval = t1->n_args == t2->n_args;
  if(retval != true)
    return retval;
  for(i = 0; i < t1->n_args; i++){
   retval = equal_terms(t1->args[i], t2->args[i]);
   if(retval != true)
     return retval;
  }
  return true;
}

bool equal_terms(const term* t1, const term* t2){
  bool retval;
  if(t1->type == variable_term)
    if(t2->type == variable_term)
      retval = t1->var->var_no == t2->var->var_no;
    else
      return false;
  else
    if(t2->type == variable_term)
      return false;
    else
      retval = t1->name == t2->name;
  if(retval == false)
    return retval;
  return equal_term_lists(t1->args, t2->args);
}

bool equal_atoms(const atom* a1, const atom* a2){
  if(a1->pred->pred_no != a2->pred->pred_no)
    return false;
  return equal_term_lists(a1->args, a2->args);
}


/**
   Generic printing facilities
**/
void print_term_list(const term_list *tl, FILE* stream, void (*print_term)(const term*, FILE*)){
  int i;
  if(tl->n_args > 0){
    fprintf(stream, "(");
    for(i = 0; i+1 < tl->n_args; i++){
      print_fol_term(tl->args[i], stream);
      fprintf(stream, ",");
    }
    print_term(tl->args[i], stream);
    fprintf(stream, ")");
  }
}
void print_atom(const atom *at, FILE* stream, void (*print_term_list)(const term_list*, FILE*)){
  fprintf(stream, "%s", at->pred->name);
  print_term_list(at->args, stream);
}
void print_term(const term *t, FILE* stream, void (*print_term_list)(const term_list*, FILE*)){
  if(t->type == variable_term)
    fprintf(stream, "%s", t->var->name);
  else
    fprintf(stream, "%s", t->name);
  print_term_list(t->args, stream);
}

/**
   Pretty printing standard fol format
**/
void print_fol_atom(const atom *at, FILE* stream){
  print_atom(at, stream, print_fol_term_list);
}

void print_fol_term_list(const term_list *tl, FILE* stream){
  print_term_list(tl, stream, print_fol_term);
}

void print_fol_term(const term *t, FILE* s){
  print_term(t, s, print_fol_term_list);
}
/**
   Printing geolog input format
**/
void print_geolog_atom(const atom *at, FILE* stream){
  print_atom(at, stream, print_fol_term_list);
}

void print_geolog_term_list(const term_list *tl, FILE* stream){
  print_term_list(tl, stream, print_fol_term);
}

void print_geolog_term(const term *t, FILE* s){
  print_term(t, s, print_fol_term_list);
}
