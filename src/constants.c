/* constants.c

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
   Constants are based on pointer equality.
**/

#include "common.h"
#include "constants.h"
#include "theory.h"
#include "error_handling.h"

constants* init_constants(unsigned int init_size){
  constants * new_c = malloc_tester(sizeof(constants));
  new_c->fresh = init_fresh_const(init_size);

  new_c->size_constants = init_size;
  new_c->constants = calloc_tester(new_c->size_constants, sizeof(constant));
  new_c->n_constants = 0;
#ifdef HAVE_PTHREAD
  pthread_mutex_init(&new_c->constants_mutex, NULL);
#endif
  return new_c;
}

void destroy_constants(constants* c){
  free(c->constants);
#ifdef HAVE_PTHREAD
  pthread_mutex_destroy(&c->constants_mutex);
#endif
  free(c);
}

constants* copy_constants(const constants * orig, timestamp_store* ts_store){
  unsigned int i;
  constants* copy = malloc_tester(sizeof(constants));
  copy = memcpy(copy, orig, sizeof(constants));
  copy->constants = calloc_tester(orig->size_constants, sizeof(constant));
  memcpy(copy->constants, orig->constants, orig->size_constants * sizeof(constant));
  assert(copy->n_constants == orig->n_constants);
  for(i = 0; i < copy->n_constants; i++)
    copy_timestamps(& copy->constants[i].steps, & orig->constants[i].steps, ts_store, false);
  return copy;
}

const char* get_constant_name(dom_elem c, const constants* cs){
  return c.name;
}

/**
   
**/
dom_elem insert_constant_name(constants * consts, const char* name){
  unsigned int new_const_ind;
  constant* new_c;
  assert(name != NULL && strlen(name) > 0);
  new_const_ind = consts->n_constants;
  consts->n_constants++;
  if(consts->size_constants <= consts->n_constants){
    consts->size_constants *= 2;
    consts->constants = realloc_tester(consts->constants, consts->size_constants * sizeof(constant));
  }
  new_c = & consts->constants[new_const_ind];
  new_c->elem.name = malloc_tester(strlen(name)+2);
  strcpy((char*) new_c->elem.name, name);
  new_c->rank = 0;
  new_c->parent = new_const_ind;
  new_c->elem.id = new_const_ind;
  init_empty_timestamp_linked_list(& new_c->steps, false);
  return new_c->elem;
}

/**
   Creates a new domain element. Called from instantiate.c
**/
const clp_term* get_fresh_constant(clp_variable* var, constants* constants){
  dom_elem new_const;
  char* name;
  const clp_term* t;
  assert(constants->fresh != NULL);
  unsigned int const_no = next_fresh_const_no(constants->fresh, var->var_no);
  constants->n_constants ++;
  name = calloc_tester(sizeof(char), strlen(var->name) + 20);
  sprintf(name, "%s_%i", var->name, const_no);
  new_const = insert_constant_name(constants, name);
  t = prover_create_constant_term(new_const);
  return t;
}

/**
   Part of union-find alg. 
   http://en.wikipedia.org/wiki/Disjoint-set_data_structure
**/
unsigned int find_constant_root(unsigned int c, constants* cs, timestamps* ts, timestamp_store* store, bool update_ts){
  if(cs->constants[c].parent != c)
    cs->constants[c].parent = find_constant_root(cs->constants[c].parent, cs, ts, store, update_ts);
  if(update_ts)
    add_timestamps(ts, & cs->constants[c].steps, store);
  return cs->constants[c].parent;
}


/**
   Part of union-find alg. 
   http://en.wikipedia.org/wiki/Disjoint-set_data_structure
**/
void union_constants(dom_elem c1, dom_elem c2, constants* consts, unsigned int step, timestamp_store* store){
  timestamps * tmp1 = malloc_tester(sizeof(timestamps));
  timestamps* tmp2 = malloc_tester(sizeof(timestamps));
  init_empty_timestamp_linked_list(tmp1, false);
  init_empty_timestamp_linked_list(tmp2, false);
#ifdef HAVE_PTHREAD
#ifdef __DEBUG_RETE_PTHREAD
  fprintf(stderr, "Locking constants (union)\n");
#endif
  pt_err(pthread_mutex_lock(& consts->constants_mutex), __FILE__, __LINE__, "union_constants: mutex_lock");
#ifdef __DEBUG_RETE_PTHREAD
  fprintf(stderr, "Locked constants (union)\n");
#endif
#endif
  unsigned int c1_root = find_constant_root(c1.id, consts, tmp1, store, true);
  unsigned int c2_root = find_constant_root(c2.id, consts, tmp2, store, true);
  if(c1_root == c2_root) {
#ifdef HAVE_PTHREAD
    pt_err(pthread_mutex_unlock(& consts->constants_mutex), __FILE__,  __LINE__, "union_constants: mutex_lock");
#ifdef __DEBUG_RETE_PTHREAD
    fprintf(stderr, "Unlocking constants (union 1)\n");
#endif
#endif
    return;
  }
  if(consts->constants[c1_root].rank < consts->constants[c2_root].rank){
    consts->constants[c1_root].parent = c2_root;
    add_equality_timestamp(& consts->constants[c1_root].steps, step, store, true);
    add_timestamps(& consts->constants[c1_root].steps, tmp2, store);
  } else {
    consts->constants[c2_root].parent = c1_root;
    add_equality_timestamp(& consts->constants[c2_root].steps, step, store, false);
    add_timestamps(& consts->constants[c2_root].steps, tmp1, store);
    if(!(consts->constants[c1_root].rank > consts->constants[c2_root].rank))
      consts->constants[c1_root].rank++;
  }
#ifdef HAVE_PTHREAD
  pt_err(pthread_mutex_unlock(& consts->constants_mutex), __FILE__,  __LINE__, "union_constants: mutex_lock");
#ifdef __DEBUG_RETE_PTHREAD
    fprintf(stderr, "Unlocking constants (union 2)\n");
#endif
#endif
  free(tmp1);
  free(tmp2);
}


/**
   Part of union-find alg. 
   http://en.wikipedia.org/wiki/Disjoint-set_data_structure

   If update_ts is true, the timestamps of the steps needed to infer the equality are added to ts, even if they are not equal
   It is assumed that the timestamps are discarded if the constants are unequal

   TODO: At the moment, locking is disabled, since I did not manage to debug a frequent deadlock.
   It might be that it is ok not to lock, since the changes done by find_constant_root could be compatible?
**/
bool equal_constants_locked(dom_elem c1, dom_elem c2, constants* consts, timestamps* ts, timestamp_store* store, bool update_ts){
  if(c1.id == c2.id)
    return true;
  unsigned int p1 = find_constant_root(c1.id, consts, ts, store, update_ts);
  unsigned int p2 = find_constant_root(c2.id, consts, ts, store, update_ts);
  return p1 == p2;
}

bool equal_constants_mt(dom_elem c1, dom_elem c2, constants* consts, timestamps* ts, timestamp_store* store, bool update_ts){
  if(c1.id == c2.id)
    return true;
#ifdef __DEBUG_RETE_PTHREAD
  fprintf(stderr, "Locking constants (equal): %i\n", (unsigned int) pthread_self());
#endif
#ifdef HAVE_PTHREAD
  pt_err(pthread_mutex_lock(& consts->constants_mutex),__FILE__,  __LINE__,  "union_constants: mutex_lock");
#ifdef __DEBUG_RETE_PTHREAD
  fprintf(stderr, "Locked constants (equal): %i\n", (unsigned int) pthread_self());
#endif
#endif
  bool rv = equal_constants_locked(c1, c2, consts, ts, store, update_ts);
#ifdef __DEBUG_RETE_PTHREAD
  fprintf(stderr, "Unlocking constants (equal): %i\n", (unsigned int) pthread_self());
#endif
#ifdef HAVE_PTHREAD
  pt_err(pthread_mutex_unlock(& consts->constants_mutex), __FILE__,  __LINE__,  "union_constants: mutex_unlock");
#ifdef __DEBUG_RETE_PTHREAD
  fprintf(stderr, "Unlocked constants (equal): %i\n", (unsigned int) pthread_self());
#endif
#endif
  return rv;
}


/**
   Called from rete_state when in a disjunctive split. 
**/
constants* backup_constants(const constants* orig, timestamp_store* ts_store){
  return copy_constants(orig,ts_store);
}

/**
   The prover iterates over all constants to get 
   the domain
**/
constants_iter get_constants_iter(constants* state){
  return 0;
}

bool constants_iter_has_next(constants* state, constants_iter* iter){
  return *iter < state->n_constants;
}

constant* constants_iter_get_next(constants* state, constants_iter *iter){
  assert(constants_iter_has_next(state, iter));
  return & state->constants[*iter++];
}

void destroy_constants_iter(constants_iter iter){
  return;
}

 
/**
   Called from the parser (via create_constant_term in atom_and_term.c) 
   when seeing a constant
**/
dom_elem parser_new_constant(constants* cs, const char* new){
  unsigned int i;
  for(i = 0; i < cs->n_constants; i++){
    constant c = cs->constants[i];
    if(strcmp(c.elem.name, new) == 0)
      return c.elem;
  }
  return insert_constant_name(cs, new);
}

bool test_constant(dom_elem c, const constants* cs){
  assert(cs->size_constants > 0);
  assert(cs->constants != NULL);
  assert(c.id < cs->n_constants);
  return true;
}

/**
   Prints all constants, with partitions
**/
void print_all_constants(constants* cs, FILE* stream){
  unsigned int partitions[cs->n_constants][cs->n_constants];
  unsigned int size_partition[cs->n_constants];
  unsigned int i;
  for(i = 0; i < cs->n_constants; i++)
    size_partition[i] = 0;
  for(i = 0; i < cs->n_constants; i++){
    unsigned int p = find_constant_root(i, cs, NULL, NULL, false);
    partitions[p][size_partition[p]] = i;
    size_partition[p]++;
  }
  for(i = 0; i < cs->n_constants; i++){
    if(size_partition[i] > 0){
      unsigned int j;
      fprintf(stream, "{");
      for(j = 0; j < size_partition[i]; j++)
	fprintf(stream, "%s ", cs->constants[partitions[i][j]].elem.name);
      fprintf(stream, "}");
    }
  }
  fprintf(stream, "\n");
}


void print_coq_constants(const constants* cs, FILE* stream){
  unsigned int i;
  if(cs->n_constants > 0){
    fprintf(stream, "Variables ");
    for(i = 0; i < cs->n_constants; i++){
      char first = cs->constants[i].elem.name[0];
      if(first - '0' >= 0 && first - '0' <= 9)
	fprintf(stream, "num_%s ", cs->constants[i].elem.name);
      else
	fprintf(stream, "%s ", cs->constants[i].elem.name);
    }
    fprintf(stream, ": %s.\n", DOMAIN_SET_NAME);
  }
}
