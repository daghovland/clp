/* substitution.c

   Copyright 2011 

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
   Auxiliary Code for updating state of rete network
**/
#include "common.h"
#include "term.h"
#include "fresh_constants.h"
#include "rete.h"
#include "theory.h"
#include "substitution.h"

bool unify_substitution_term_lists(const term_list*, const term_list*, substitution*);



/**
   Substitution constructor and destructor
**/
substitution* create_substitution(const theory* t, unsigned int timestamp){
  unsigned int i;
  substitution* ret_val = malloc_tester(sizeof(substitution) + t->vars->n_vars * sizeof(term*) );
  ret_val->allvars = t->vars;
  ret_val->n_subs = 0;

  ret_val->size_timestamps = t->max_lhs_conjuncts;
  ret_val->timestamps = calloc_tester(sizeof(unsigned int), ret_val->size_timestamps);
  ret_val->timestamps[0] = timestamp;
  ret_val->n_timestamps = 1;

  for(i = 0; i < t->vars->n_vars; i++)
    ret_val->values[i] = NULL;
  return ret_val;
}

substitution* copy_substitution(const substitution* orig){
  size_t size = sizeof(substitution) + orig->allvars->n_vars * sizeof(term*);

  substitution* copy = malloc_tester(size );
  memcpy(copy, orig, size);
  
  copy->timestamps = calloc_tester(sizeof(unsigned int), copy->size_timestamps);
  memcpy(copy->timestamps, orig->timestamps, copy->size_timestamps * sizeof(unsigned int));

  assert(test_substitution(orig));
  assert(test_substitution(copy));

  return copy;
}

void delete_substitution(substitution* a){
  free(a);
}


const term* find_substitution(const substitution* sub, const variable* key){
  assert(test_substitution(sub));
  if(sub == NULL)
    return NULL;
  return  sub->values[key->var_no];
}

/**
   Adds one more key/value pair to a substitution, 
   if it does not already occur. 

   Fails if the key occurs with a different value
**/
bool _add_substitution_no(substitution* sub, size_t var_no, const term* value){
  const term* orig_val = sub->values[var_no];

  assert(test_term(value));

  if(orig_val == NULL){
    sub->n_subs++;
    sub->values[var_no] = value;
    
    assert(test_substitution(sub));

    return true;
  }
  assert(test_term(orig_val));
  return equal_terms(value, orig_val);
}



bool add_substitution(substitution* sub, variable* var, const term* value){
  return _add_substitution_no(sub, var->var_no, value);
}


/**
   Inserts a key/value pair into a substitution.

   Does not fail if the key already occurs with a different value
**/
void insert_substitution_value(substitution* sub, variable* var, const term* value){
  size_t var_no = var->var_no;
  assert(test_term(value));
  if(sub->values[var_no] == NULL)
    sub->n_subs++;
  sub->values[var_no] = value;
  
  assert(test_substitution(sub));
}

/**
   Unify an substitution with one more key/value pair
   Used when inserting into alpha node

   value comes from the inserted fact, 
   while argument comes from the rule / rete node
**/

bool unify_substitution_terms(const term* value, const term* argument, substitution* sub){
  freevars* free_arg_vars = init_freevars();

  assert(test_substitution(sub));
  assert(test_term(value));
  assert(test_term(argument));

  free_arg_vars = free_term_variables(argument, free_arg_vars);
  switch(argument->type){
  case variable_term: 
    return add_substitution(sub, argument->var, value);
  case constant_term:
    return (value->type == constant_term && strcmp(value->name, argument->name) == 0);
    break;
  case function_term:
    return (value->type == function_term 
	    && value->name == argument->name 
	    && unify_substitution_term_lists(value->args, argument->args, sub
					     ));
    break;
  default:
    fprintf(stderr, "Unknown term type %i occurred\n", argument->type);
    abort();
  }
  return false;
} 

bool unify_substitution_term_lists(const term_list* value, const term_list* arg, substitution* sub){
  unsigned int i;

  assert(test_substitution(sub));

  if(value->n_args != arg->n_args)
    return false;
  for(i = 0; i < arg->n_args; i++){
    if(! unify_substitution_terms(value->args[i], arg->args[i], sub))
      return false;
  }
  return true;
}

/**
   Tests whether two substitutions map the intersection of their domains to the same values
**/
bool subs_equal_intersection(const substitution* sub1, const substitution* sub2){
  unsigned int i;

  assert(test_substitution(sub1));
  assert(test_substitution(sub2));


  for(i = 0; i < sub1->allvars->n_vars; i++){
    const term* val1 = sub1->values[i];
    const term* val2 = sub2->values[i];
    if(val1 != NULL && val2  != NULL && !equal_terms(val1, val2))
	return false;
  }
  return true;
}

/**
   Tests that a substitution is correct
**/
bool test_substitution(const substitution* sub){
  unsigned int i, c;
  assert(sub != NULL);
  assert(sub->n_subs <= sub->allvars->n_vars);

  c = 0;
  for(i = 0; i < sub->allvars->n_vars; i++){
    if(sub->values[i] != NULL){
      c++;
      assert(test_term(sub->values[i]));
    }
  }

  assert(c == sub->n_subs);
  return true;
}
/**
   Tests that all variables in fv has a value in sub
**/
bool test_is_instantiation(const freevars* fv, const substitution* sub){
  freevars_iter iter = get_freevars_iter(fv);
  while(has_next_freevars_iter(&iter)){
    variable* var = next_freevars_iter(&iter);
    if(find_substitution(sub, var) == NULL)
      return false;
  }
  return true;
}

/**
   Creates a new substitution on the heap which is the union of two substitutions

   If this is not possible, because they map variables in the intersection of the domain to
   different values, then NULL is returned
**/
substitution* union_substitutions(const substitution* sub1, const substitution* sub2){
  unsigned int i, new_size;
  substitution *retval;

  assert(test_substitution(sub1));
  assert(test_substitution(sub2));
  assert(sub1->allvars == sub2->allvars);

  retval = copy_substitution(sub1);
  new_size = retval->n_timestamps + sub2->n_timestamps;
  if(retval->size_timestamps <= new_size){
    while(retval->size_timestamps <= new_size)
      retval->size_timestamps *= 2;
    retval->timestamps = realloc_tester(retval->timestamps, retval->size_timestamps * sizeof(unsigned int));
  }
  for(i = 0; i < sub2->n_timestamps; i++)
    retval->timestamps[retval->n_timestamps++] = sub2->timestamps[i];

  assert(retval->n_timestamps <= retval->size_timestamps);


  for(i = 0; i < retval->allvars->n_vars; i++){
    const term* val1 = retval->values[i];
    const term* val2 = sub2->values[i];
    if(val1 == NULL){
      if(val2 != NULL){
	retval->n_subs++;
	retval->values[i] = val2;
      }
    } else if(val2 != NULL && !equal_terms(val1, val2)){
      delete_substitution(retval);
      assert(! subs_equal_intersection(sub1, sub2));
      return NULL;
    }
  }
  
  assert(subs_equal_intersection(sub1, sub2));
  assert(test_substitution(retval));
  
  return retval;
}

/**
   Test whether two substitutions are the same for the set of variables given
**/
bool equal_substitutions(const substitution* a, const substitution* b, const freevars* vars){
  unsigned int i;
  assert(test_substitution(a));
  assert(test_substitution(b));
  for(i = 0; i < vars->n_vars; i++){
    const term* t1 = find_substitution(a, vars->vars[i]);
    const term* t2 = find_substitution(b, vars->vars[i]);
    if( t1 == NULL && t2 == NULL)
      continue;
    if ( t1 != NULL && t2 != NULL && equal_terms(t1, t2))
      continue;
    return false;
  }
  return true;
}

void delete_substitution_list(substitution_list* sub_list){
  while(sub_list != NULL){
    substitution_list* next = sub_list->next;
    delete_substitution(sub_list->sub);
    free(sub_list);
    sub_list = next;
  }
}

void delete_substitution_list_below(substitution_list* list, substitution_list* limit){
  assert(limit != NULL);
  while(list != limit){
    assert(list != NULL);
    substitution_list* next = list->next;
    delete_substitution(list->sub);
    free(list);
    list = next;
  }
}

/**
   Iterator functionality

   An iterator is just a pointer to a substitution_list. 
   A pointer to an iterator therefore a pointer to a pointer to a substitution_list

   The substitutions must be returned in order: The oldest first. Therefore the
   prev pointer is used. This is not thread safe!!!
**/
sub_list_iter* get_sub_list_iter(substitution_list* sub){
  sub_list_iter* i = malloc_tester(sizeof(sub_list_iter));
  substitution_list* prev = NULL;

  while(sub != NULL){
    sub->prev = prev;
    prev = sub;    
    sub = sub->next;
  }
  *i = prev;
    
  return i;
}


bool has_next_sub_list(const sub_list_iter*i){
  return *i != NULL;
}

/**
   As mentioned above, this is not thread safe, since
   subsitution lists could be shared between threads, and
   prev can be different. prev is set to NULL as soon as possible 
   to ensure early segfaults on erroneous usage
**/
substitution* get_next_sub_list(sub_list_iter* i){
  substitution *retval;
  substitution_list * sl = *i;
  assert(*i != NULL);
  retval = sl->sub;
  (*i) = sl->prev;
  sl->prev = NULL;
  return retval;
}

/**
   Insert substition into list, if not already there
   Used when inserting into alpha- and beta-stores and rule-stores.

   This function creates a new "substititution_list" element on the heap
   This element must be deleted when backtracking by calling delete_substitution_list_below

   The actual substitution is put on the list, and must not be changed or deleted afterwards.

   Note that for rule nodes, the same substitution is put in the rule queue, and
   pointer equality of these substitutions is important for later removal. 
**/
bool insert_substitution(rete_net_state* state, size_t sub_no, substitution* a, const freevars* free_vars){
  substitution_list* sub_list = state->subs[sub_no];

  assert(test_substitution(a));
  assert(state->net->n_subs >= sub_no);

  while(sub_list != NULL && sub_list->sub != NULL){
    if(equal_substitutions(a, sub_list->sub, free_vars))
      return false;
    sub_list = sub_list->next;
  }
  sub_list = state->subs[sub_no];
  state->subs[sub_no] = malloc_tester(sizeof(substitution_list));
  state->subs[sub_no]->sub = a;
  state->subs[sub_no]->next = sub_list;

  assert(test_substitution(a));
  
  return true;
}

/**
   Output functions
**/
void print_substitution(const substitution* sub, FILE* f){
  size_t i;
  size_t j = 0;
  fprintf(f,"[");
  if(sub != NULL){
    for(i = 0; i < sub->allvars->n_vars; i++){
      if(sub->values[i] != NULL){
	print_variable(sub->allvars->vars[i],f);
	fprintf(f, "->");
	print_fol_term(sub->values[i],f);
	if(j < sub->n_subs-1)
	  fprintf(f, ",");
	j++;
      }
    }
  }
  fprintf(f, "] from steps ");
  for(i = 0; i < sub->n_timestamps; i++)
    fprintf(f, "%u, ", sub->timestamps[i]);
}

void print_substitution_list(const substitution_list* sublist, FILE* f){
  if(sublist != NULL){
    if(sublist->sub != NULL)
      print_substitution(sublist->sub, f);
    fprintf(f, ", ");
    if(sublist->next != NULL)
      print_substitution_list(sublist->next, f);
  }
}
