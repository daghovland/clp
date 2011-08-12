/* instantiate.c

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
#include "rete.h"
#include "substitution.h"


/**
   Tests that a substitution has a value for all variables in the conjunction
**/
bool test_is_conj_instantiation(const conjunction* a, const substitution* sub){
  freevars* fv = init_freevars();
  fv = free_conj_variables(a, fv);
  assert(test_is_instantiation(fv, sub));
  del_freevars(fv);
  return true;
}


/**
   Instantiating an atom with a substitution
**/
const term* instantiate_term(const term* orig, const substitution* sub){
  unsigned int i;
  const term* t;
  switch(orig->type){
  case constant_term: 
    t = orig;
    break;
  case variable_term:
    t = find_substitution(sub, orig->var);
    break;
  case function_term: 
    t =  create_term(orig->name, instantiate_term_list(orig->args, sub));
    break;
  default:
    fprintf(stderr, "Unknown term type %i occurred\n", orig->type);
    assert(false);
  }
  assert(test_term(t));
  return t;
}

/**
   Deletes a term instantiated by the function above
   called from the similar function for term lists below
**/
void delete_instantiated_term(const term* orig, term* copy){
  switch(copy->type){
  case constant_term:
    break;
  case variable_term:
    break;
  case function_term:
    delete_instantiated_term_list(orig->args, (term_list*) copy->args);
    free(copy);
    break;
  default:
    fprintf(stderr, "Unknown term type %i occurred\n", orig->type);
    assert(false);
  }
}

/**
   Deletes a term list instantiated by the function above
   called from the similar function for atoms below
**/
const term_list* instantiate_term_list(const term_list* orig, 
				       const substitution* sub){
  unsigned int i;
  const term * t = instantiate_term(orig->args[0], sub);
  term_list* retval = create_term_list(t);
  for(i = 1; i < orig->n_args; i++)
    extend_term_list(retval, instantiate_term(orig->args[i], sub));
  assert(test_term_list(retval));
  return retval;
}

void delete_instantiated_term_list(const term_list* orig, term_list* copy){
  unsigned int i;
  if(copy->args != NULL){
    for(i = 0; i < copy->n_args; i++){
      if(copy->args[i] != NULL)
	delete_instantiated_term(orig->args[i], (term*) copy->args[i]);
    }
    free(copy->args);
  }
  free(copy);
}

/**
   Tests that a substitution has a value for all variables in the atom
**/
bool test_is_atom_instantiation(const atom* a, const substitution* sub){
  freevars* fv = init_freevars();
  fv = free_atom_variables(a, fv);
  assert(test_is_instantiation(fv, sub));
  del_freevars(fv);
  return true;
}

atom* instantiate_atom(const atom* orig, const substitution* sub){
  assert(test_atom(orig));
  assert(test_substitution(sub));
  if (orig->args->n_args > 0){
    atom* retval;
    const term_list* ground_args = instantiate_term_list(orig->args, sub);
    assert(test_term_list(ground_args));
    retval = prover_create_atom(orig->pred, ground_args);
    assert(test_atom(retval));
    return (atom*) retval;
  } else 
    return (atom*) orig;
}

/**
   Deletes the atom "copy" instantiated by the function above. 
   Called from prover. 
**/
void delete_instantiated_atom(const atom* orig, atom* copy){
  if(copy->args->n_args > 0){
    if(copy->args != NULL){
      delete_instantiated_term_list(orig->args, copy->args);
    }
    free(copy);
  }
}
/**
   Extends the substitution with fresh constants for the
   existentially bound variables
**/
void fresh_exist_constants(rete_net_state* state, const conjunction* con, substitution* sub){
  freevars_iter exist_iter = get_freevars_iter(con->bound_vars);
  while(has_next_freevars_iter(&exist_iter)){
    variable* var = next_freevars_iter(&exist_iter);
    const term* t =  get_fresh_constant(state, var);
    insert_substitution_value(sub, var, t);
  }
}
