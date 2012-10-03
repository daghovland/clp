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
#include "error_handling.h"
#include "substitution.h"
#include "substitution_store_mt.h"
#include "substitution_size_info.h"

// Defined in main.c
extern bool use_substitution_store;

/**
   Allocates memory on the heap for a substitution including all substructures
   Mostly used for temporary substitutions
**/
substitution* alloc_heap_substitution(substitution_size_info ssi){
  substitution* tmp_sub = malloc(get_size_substitution(ssi));
  return tmp_sub;
}

/**
   Allocates memory for a substitution according to the gloabl variable use_substitution_store
**/
substitution* get_substitution_memory(substitution_size_info ssi, substitution_store_mt* store){
  substitution* ret_val;
  if(use_substitution_store)
    ret_val = get_substitution_store_mt(store, ssi);
  else {
    ret_val = alloc_heap_substitution(ssi);
  }
  ret_val->sub_values_offset = get_sub_values_offset(ssi);
  return ret_val;
}

/**
   Frees a substitution and all substructures.
   Assumes that it is allocated on the heap.
   Note that most substitutions are not allocated directly on the heap,
   but in a substitution_store
**/
void free_substitution(substitution* sub){
  if(!use_substitution_store)
    free(sub);
}


/**
   Gets the term at index no of the values
   Must only be accessed through these getters and setters, 
   since there is an offset from the timestamps which has a flexible array member
**/
const clp_term* get_sub_value(const substitution* sub, unsigned int no){
  const clp_term* const * values = sub->sub_values;
  values += sub->sub_values_offset;
  return values[no];
  //  return (sub->sub_values + sub->sub_values_offset)[no];
}

void set_sub_value(substitution* sub, unsigned int no, const clp_term* t){
  (sub->sub_values + sub->sub_values_offset)[no] = t;
}

/**
   Called when initializing a rule instance. Stores the actual pointer of the
   substitution values array, such that these can be accessed without 
   dereferencing anything else in the substitution
*/
const clp_term* const * get_sub_values_ptr(const substitution* sub){
  return sub->sub_values + sub->sub_values_offset;
}

/**
   Initializes already allocated substitution
   Only used directly by the factset_lhs implementation 
   in axiom_false_in_fact_set in rete_state.c
**/
void init_empty_substitution(substitution* sub, const theory* t){
  unsigned int i;    
  sub->allvars = t->vars;
  sub->n_subs = 0;
  init_empty_timestamps(& sub->sub_ts, t->sub_size_info);
  for(i = 0; i < t->vars->n_vars; i++)
    set_sub_value(sub, i, NULL);
}


/**
   Substitution constructor and destructor.
   Only used directly by the factset_lhs implementation 
   in axiom_false_in_fact_set in rete_state.c
**/
substitution* create_empty_substitution(const theory* t, substitution_store_mt* store){
  substitution* ret_val = get_substitution_memory(t->sub_size_info, store);
  init_empty_substitution(ret_val, t);
  return ret_val;
}

void init_substitution(substitution* sub, const theory* t, signed int timestamp, timestamp_store* store){
  init_empty_substitution(sub, t);
  add_sub_timestamp(sub, timestamp, t->sub_size_info, store);
}

/**
   Substitution constructor and destructor
**/
substitution* create_substitution(const theory* t, signed int timestamp, substitution_store_mt* store, timestamp_store* ts_store, const constants* cs){
  substitution* ret_val = get_substitution_memory(t->sub_size_info, store);
  init_substitution(ret_val, t, timestamp, ts_store);
  assert(test_substitution(ret_val, cs));
  return ret_val;
}


/**
   Only called from prover() in prover.c, when creating empty substitutions for facts.
   Inserts a number needed by the coq proof output.
**/
substitution* create_empty_fact_substitution(const theory* t, const clp_axiom* a, substitution_store_mt* store, timestamp_store* ts_store, const constants* cs){
  substitution* sub = create_substitution(t, 1, store, ts_store, cs);
  assert(test_substitution(sub, cs));
  return sub;
}

/**
   Copies a substitution, including terms and timestamps from orig to dest. 
   Assumes dest  memory is already allocated. 
   dest->sub_ts is overwritten. The previous timestamps are not deallocated, since the 
   timestamp store takes care of this when backtracking
   Overwrites dest.
**/
void copy_substitution_struct(substitution* dest, const substitution* orig, substitution_size_info ssi, timestamp_store* store, bool permanent, const constants* cs){
  assert(test_substitution(orig, cs));
  memcpy(dest, orig, get_size_substitution(ssi));
  copy_timestamps(& dest->sub_ts, & orig->sub_ts, store, permanent);
  assert(test_substitution(dest, cs));
}


/**
   Creates a new substitution, either on the heap, or on
   the store
**/
substitution* copy_substitution(const substitution* orig, substitution_store_mt* store, substitution_size_info ssi, timestamp_store* ts_store, const constants* cs){
  substitution* copy;
  
  if(use_substitution_store)
    copy = get_substitution_store_mt(store, ssi);
  else 
    copy = alloc_heap_substitution(ssi);
  
  copy_substitution_struct(copy, orig, ssi, ts_store, false, cs);

  assert(test_substitution(orig, cs));
  assert(test_substitution(copy, cs));
  assert(orig->allvars == copy->allvars);

  return copy;
}




const clp_term* find_substitution(const substitution* sub, const clp_variable* key, const constants* cs){
  assert(test_substitution(sub, cs));
  if(sub == NULL)
    return NULL;
  return get_sub_value(sub, key->var_no);
}

/**
   Adds one more key/value pair to a substitution, 
   if it does not already occur. 

   Fails if the key occurs with a different value

   If update_ts is true, the substitution is updated with new timestamps also 
   when the equality fails. It is assumed that the substitution is then discarded
**/
bool _add_substitution_no(substitution* sub, size_t var_no, const clp_term* value, constants* cs, timestamp_store* store, bool update_ts){
  const clp_term* orig_val = get_sub_value(sub, var_no);

  assert(test_term(value, cs));

  if(orig_val == NULL){
    sub->n_subs++;
    set_sub_value(sub, var_no, value);
    
    assert(test_substitution(sub, cs));

    return true;
  }
  assert(test_term(orig_val, cs));
  return equal_terms(value, orig_val, cs, &sub->sub_ts, store, update_ts);
}



bool add_substitution(substitution* sub, clp_variable* var, const clp_term* value, constants* cs, timestamp_store* store, bool update_ts){
  return _add_substitution_no(sub, var->var_no, value, cs, store, update_ts);
}


/**
   Inserts a key/value pair into a substitution.

   Does not fail if the key already occurs with a different value
**/
void insert_substitution_value(substitution* sub, clp_variable* var, const clp_term* value, const constants* cs){
  size_t var_no = var->var_no;
  assert(test_term(value, cs));
  if(get_sub_value(sub, var_no) == NULL)
    sub->n_subs++;
  set_sub_value(sub, var_no, value);
  
  assert(test_substitution(sub, cs));
}

/**
   Unify an substitution with one more key/value pair
   Used when inserting into alpha node

   value comes from the inserted fact, 
   while argument comes from the rule / rete node

   The substitution is updated with timestamps in either case, 
   as it is discarded if there is no unification

   TODO:
   11 Aug 2011: The freevars are commented because they seem not to be used. 
   I cannot remember now what they are used for, and they were not deleted elsewhere
**/

bool unify_substitution_terms(const clp_term* value, const clp_term* argument, substitution* sub, constants* cs, timestamp_store* store){
  //  freevars* free_arg_vars = init_freevars();

  assert(test_substitution(sub, cs));
  assert(test_term(value, cs));
  assert(test_term(argument, cs));

  //free_arg_vars = free_term_variables(argument, free_arg_vars);
  switch(argument->type){
  case variable_term: 
    //del_freevars(free_arg_vars);
    return add_substitution(sub, argument->val.var, value, cs, store, true);
  case constant_term:
    //del_freevars(free_arg_vars);
    return (value->type == constant_term && equal_constants_mt(value->val.constant, argument->val.constant, cs, &sub->sub_ts, store, true));
    break;
  case function_term:
    //del_freevars(free_arg_vars);
    return (value->type == function_term 
	    && value->val.function == argument->val.function 
	    && unify_substitution_term_lists(value->args, argument->args, sub, cs, store
					     ));
    break;
  default:
    fprintf(stderr, "%s: line %i: Unknown term type %i occurred\n", __FILE__, __LINE__, argument->type);
    assert(false);
    abort();
  }
  //del_freevars(free_arg_vars);
  return false;
} 

bool unify_substitution_term_lists(const term_list* value, const term_list* arg, substitution* sub, constants* cs, timestamp_store* store){
  unsigned int i;

  assert(test_substitution(sub, cs));

  if(value->n_args != arg->n_args)
    return false;
  for(i = 0; i < arg->n_args; i++){
    if(! unify_substitution_terms(value->args[i], arg->args[i], sub, cs, store))
      return false;
  }
  return true;
}


/**
   Tests whether two substitution value lists map the intersection of their domains to the same values
**/
bool sub_value_lists_equal_intersection(const clp_term*const * value_list1, const clp_term* const * value_list2, unsigned int n_vars, constants* cs){
  unsigned int i;

  for(i = 0; i < n_vars; i++){
    const clp_term* val1 = value_list1[i];
    const clp_term* val2 = value_list2[i];
    if(val1 != NULL && val2  != NULL && !equal_terms(val1, val2, cs, NULL, NULL, false))
	return false;
  }
  return true;
}

/**
   Tests whether two substitutions map the intersection of their domains to the same values
**/
bool subs_equal_intersection(const substitution* sub1, const substitution* sub2, constants* cs){
  unsigned int i;

  assert(test_substitution(sub1, cs));
  assert(test_substitution(sub2, cs));

  return sub_value_lists_equal_intersection(get_sub_values_ptr(sub1), get_sub_values_ptr(sub2), sub1->allvars->n_vars, cs);
  /*for(i = 0; i < sub1->allvars->n_vars; i++){
    const clp_term* val1 = get_sub_value(sub1, i);
    const clp_term* val2 = get_sub_value(sub2, i);
    if(val1 != NULL && val2  != NULL && !equal_terms(val1, val2, cs, NULL, NULL, false))
	return false;
  }
  return true;*/
}



/**
   Tests that a substitution is correct
**/
bool test_substitution(const substitution* sub, const constants* cs){
  unsigned int i, c;
  assert(sub != NULL);
  assert(sub->n_subs <= sub->allvars->n_vars);
  
  c = 0;
  for(i = 0; i < sub->allvars->n_vars; i++){
    const clp_term* t = get_sub_value(sub, i);
    if(t != NULL){
      c++;
      assert(test_term(t, cs));
    }
  }

  assert(c == sub->n_subs);
  assert(test_timestamps(& sub->sub_ts));
  return true;
}
/**
   Tests that all variables in fv has a value in sub
**/
bool test_is_instantiation(const freevars* fv, const substitution* sub, const constants* cs){
  freevars_iter iter = get_freevars_iter(fv);
  while(has_next_freevars_iter(&iter)){
    clp_variable* var = next_freevars_iter(&iter);
    if(find_substitution(sub, var, cs) == NULL)
      return false;
  }
  return true;
}

/**
   If orig and dest equal in the overlap, adds all of origs terms to dest and returns true
   Otherwise returns false. Does not allocate or deallocate any memory.

   Called from the different union_substitution... functions below

   Timestamps of dest are updated in any case.
**/
bool union_substitutions_struct_terms(substitution* dest, const substitution* orig, constants* cs, timestamp_store* store){
  unsigned int i;
  for(i = 0; i < dest->allvars->n_vars; i++){
    const clp_term* val1 = get_sub_value(dest, i);
    const clp_term* val2 = get_sub_value(orig, i);
    if(val1 == NULL){
      if(val2 != NULL){
	dest->n_subs++;
	set_sub_value(dest, i, val2);
      }
    } else if(val2 != NULL && !equal_terms(val1, val2, cs, &dest->sub_ts, store, true)){
      assert(! subs_equal_intersection(dest, orig, cs));
      return false;
    }
  }
  return true;
}

/**
   If sub1 and sub2 have overlapping intersection, makes dest the union and returns true
   Otherwise returns false.

   Only timestamps from sub1 is kept.
   This is used by the rhs-part of the rete network.

   Assumes dest is already allocated. 
   Timestamps of dest are updated. Must not be used if return-value is false
**/
bool union_substitutions_struct_one_ts(substitution* dest, const substitution* sub1, const substitution* sub2, substitution_size_info ssi, constants* cs, timestamp_store* ts_store){
  copy_substitution_struct(dest, sub1, ssi, ts_store, false, cs);
  return union_substitutions_struct_terms(dest, sub2, cs, ts_store);
}

/**
   Creates a new substitution on the heap which is the union of two substitutions

   If this is not possible, because they map variables in the intersection of the domain to
   different values, then NULL is returned

   The timestamps are only those of sub1. sub2 timestamps are not part of the 
   new substitution
**/
substitution* union_substitutions_one_ts(const substitution* sub1, const substitution* sub2, substitution_store_mt* store, substitution_size_info ssi, constants* cs, timestamp_store* ts_store){
  substitution *retval;

  assert(test_substitution(sub1, cs));
  assert(test_substitution(sub2, cs));
  assert(sub1->allvars == sub2->allvars);
  
  if(use_substitution_store)
    retval = get_substitution_store_mt(store, ssi);
  else {
    retval = alloc_heap_substitution(ssi);
  }
  
  if(!union_substitutions_struct_one_ts(retval, sub1, sub2, ssi, cs, ts_store)){
    if(!use_substitution_store)
      free_substitution(retval);
    return NULL;
  }
  
  assert(subs_equal_intersection(sub1, sub2, cs));
  assert(test_substitution(retval, cs));
  
  return retval;
}

/**
   If sub1 and sub2 have overlapping intersection, makes dest the union and returns true
   Otherwise returns false.
   Also copies the timestamps of sub2

   Assumes dest is already allocated
**/
bool union_substitutions_struct_with_ts(substitution* dest, const substitution* sub1, const substitution* sub2, substitution_size_info ssi, constants* cs, timestamp_store* store){
  if(! union_substitutions_struct_one_ts(dest, sub1, sub2, ssi, cs, store))
    return false;
#ifdef __DEBUG_RETE_PTHREAD
  fprintf(stderr, "Locking constants (subs): %i\n", pthread_self());
#endif
#ifdef HAVE_PTHREAD
  pt_err(pthread_mutex_lock(& cs->constants_mutex), __FILE__, __LINE__, "union_substitutions_struct_with_ts: mutex_lock");
#endif
#ifdef __DEBUG_RETE_PTHREAD
  fprintf(stderr, "Locked constants (subs): %i\n", pthread_self());
#endif
  add_timestamps(& dest->sub_ts, & sub2->sub_ts, store);
#ifdef __DEBUG_RETE_PTHREAD
  fprintf(stderr, "Unlocking constants (subs): %i\n", pthread_self());
#endif
#ifdef HAVE_PTHREAD
  pt_err(pthread_mutex_unlock(& cs->constants_mutex), __FILE__, __LINE__, "union_substitutions_struct_with_ts: mutex_unlock");
#ifdef __DEBUG_RETE_PTHREAD
  fprintf(stderr, "Unlocked constants (subs): %i\n", pthread_self());
#endif
#endif
  return true;
}

/**
   Creates a new substitution on the heap which is the union of two substitutions

   If this is not possible, because they map variables in the intersection of the domain to
   different values, then NULL is returned
**/
substitution* union_substitutions_with_ts(const substitution* sub1, const substitution* sub2, substitution_store_mt* store, substitution_size_info ssi, constants* cs, timestamp_store* ts_store){
  
  substitution *retval;

  if(use_substitution_store)
    retval = get_substitution_store_mt(store, ssi);
  else {
    retval = alloc_heap_substitution(ssi);
  }

  if(!union_substitutions_struct_with_ts(retval, sub1, sub2, ssi, cs, ts_store)){
    if(!use_substitution_store)
      free_substitution(retval);
    return NULL;
  }
  return retval;  
}

/**
   Test whether two substitutions are the same for the set of variables given

   if literally is true, the substitutions must give same name constants to same variable, 
   otherwise a semantic equality is used

   The timestamps are not added 
**/
bool _equal_substitutions(const substitution* a, const substitution* b, const freevars* vars, constants* cs, bool literally){
  unsigned int i;
  assert(test_substitution(a, cs));
  assert(test_substitution(b, cs));
  for(i = 0; i < vars->n_vars; i++){
    const clp_term* t1 = find_substitution(a, vars->vars[i], cs);
    const clp_term* t2 = find_substitution(b, vars->vars[i], cs);
    if( t1 == NULL && t2 == NULL)
      continue;
    if ( t1 != NULL && t2 != NULL && (literally ? literally_equal_terms(t1, t2, cs, NULL, NULL, false) : equal_terms(t1, t2, cs, NULL, NULL, false)))
      continue;
    return false;
  }
  return true;
}

bool equal_substitutions(const substitution* a, const substitution* b, const freevars* vars, constants* cs){
  return _equal_substitutions(a, b, vars, cs, false);
}

bool literally_equal_substitutions(const substitution* a, const substitution* b, const freevars* vars, constants* cs){
  return _equal_substitutions(a, b, vars, cs, true);
}

void delete_substitution_list(substitution_list* sub_list){
  while(sub_list != NULL){
    substitution_list* next = sub_list->next;
    free(sub_list);
    sub_list = next;
  }
}

void delete_substitution_list_below(substitution_list* list, substitution_list* limit){
  while(list != limit){
    substitution_list* next = list->next;
    free(list);
    list = next;
  }
}



/**
   Iterator functionality

   An iterator is just a pointer to a substitution_list. 
   A pointer to an iterator therefore a pointer to a pointer to a substitution_list

   The substitutions must be returned in order: The oldest first. Therefore the
   prev pointer is used. This is not thread safe. Therefore the accessor function 
   must use a mutex. This is done in rete_state.c
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

void free_sub_list_iter(sub_list_iter* i){
  free(i);
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
bool insert_substitution(rete_net_state* state, unsigned int sub_no, substitution* a, const freevars* free_vars, constants* cs){
  substitution_list* sub_list = state->subs[sub_no];

  assert(test_substitution(a, cs));
  assert(state->net->n_subs >= sub_no);

  while(sub_list != NULL && sub_list->sub != NULL){
    if(equal_substitutions(a, sub_list->sub, free_vars, cs))
      return false;
    sub_list = sub_list->next;
  }
  sub_list = state->subs[sub_no];
  state->subs[sub_no] = malloc_tester(sizeof(substitution_list));
  state->subs[sub_no]->sub = a;
  state->subs[sub_no]->next = sub_list;

  assert(test_substitution(a, cs));
  
  return true;
}

/**
   Adds a single timestamp to the list in the substitution

   Called from reamining_axiom_false_in_factset in rete_state.c

   Necessary for the output of correct coq proofs
**/
void add_sub_timestamp(substitution* sub, unsigned int timestamp, substitution_size_info ssi, timestamp_store* store){  
  add_normal_timestamp(& sub->sub_ts, timestamp, store);
}


unsigned int get_sub_n_timestamps(const substitution* sub){
  return get_n_timestamps(& sub->sub_ts);
}
timestamps_iter get_sub_timestamps_iter(const substitution* sub){
  return get_timestamps_iter(& sub->sub_ts);
}

int compare_sub_timestamps(const substitution* s1, const substitution* s2){
  return compare_timestamps(& s1->sub_ts, & s2->sub_ts);
}

/**
   Output functions
**/
void print_substitution(const substitution* sub, const constants* cs, FILE* f){
  unsigned int i;
  timestamps_iter iter;
  unsigned int j = 0;
  fprintf(f,"[");
  if(sub != NULL){
    for(i = 0; i < sub->allvars->n_vars; i++){
      if(get_sub_value(sub, i) != NULL){
	print_variable(sub->allvars->vars[i],f);
	fprintf(f, "->");
	print_fol_term(get_sub_value(sub, i),cs, f);
	if(j < sub->n_subs-1)
	  fprintf(f, ",");
	j++;
      }
    }
  }
  fprintf(f, "] from steps ");
  iter = get_sub_timestamps_iter(sub);
  while(has_next_timestamps_iter(&iter))
    fprintf(f, "%u, ", get_next_timestamps_iter(&iter).step);
}

void print_coq_substitution(const substitution* sub, const constants* cs, const freevars* vars, FILE* f){
  unsigned int i;
  for(i = 0; i < vars->n_vars; i++){
    fprintf(f, " ");
    print_coq_term(get_sub_value(sub, vars->vars[i]->var_no),cs, f);
  }
}

void print_substitution_list(const substitution_list* sublist, const constants* cs, FILE* f){
  if(sublist != NULL){
    if(sublist->sub != NULL)
      print_substitution(sublist->sub, cs, f);
    fprintf(f, ", ");
    if(sublist->next != NULL)
      print_substitution_list(sublist->next, cs, f);
  }
}
