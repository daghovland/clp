/* timestamp_linked_list.c

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
   Arrays of timestamps are used in substitutions, to keep
   track of the steps at which the left-hand side were first inferred.

   Included from timestamp_list _if_ USE_TIMESTAMP_ARRAY is defined in common.h
   Otherwise, timestamp_linked_list is used

   If there are n timestamps, there are n elements in the linked list. The oldest/first inserted has next pointer NULL
   the other n-1 have valid next pointer.
   
   New timestamps are added at the end "list"
   But union of timestamps is done by concatenation, so the order of the timestamps is not necessarily timestamp-ordered
**/
#include "common.h"
#ifdef USE_TIMESTAMP_ARRAY
#abort
#endif
#include "timestamps.h"
#include "term.h"
#include "substitution.h"
#include "rule_instance.h"
#include "error_handling.h"
#ifdef HAVE_PTHREAD
#include <pthread.h>
#endif


/**
   Initializes already allocated substitution
   Only used directly by the factset_lhs implementation 
   in axiom_false_in_fact_set in rete_state.c

   Assumes the memory for the timestamp is already allocated
**/
void init_empty_timestamps(timestamps* ts, substitution_size_info ssi){
  ts->list = NULL;
  ts->first = NULL;
}

timestamp get_first_timestamp(timestamps* ts){
  assert(ts != NULL && ts->list != NULL && ts->first != NULL);
  return ts->first->ts;
}

timestamps_iter get_timestamps_iter(const timestamps* ts){
  timestamps_iter iter;
  iter.ts_list = ts->list;
  return iter;
}

bool has_next_timestamps_iter(const timestamps_iter* iter){
  return iter->ts_list != NULL;
}

timestamp get_next_timestamps_iter(timestamps_iter* iter){
  assert(iter != NULL);
  timestamp ts = iter->ts_list->ts;
  iter->ts_list = iter->ts_list->next;
  return ts;
}

void destroy_timestamps_iter(timestamps_iter* iter){
  ;
}
 
unsigned int get_n_timestamps(const timestamps* ts){
  unsigned int n = 0;
  timestamp_linked_list * el = ts->list;
  while(el != NULL){
    el = el->next;
    n++;
  }
  return n;
}



timestamp_linked_list* get_timestamp_memory(timestamp_store* store){
  timestamp_linked_list* retval;
#ifdef HAVE_PTHREAD
  pt_err(pthread_mutex_lock(& store->lock), "#__FILE__ : #__LINE__: mutex lock");
#endif
  if(store->n_timestamp_store >= store->size_timestamp_store){
    store->n_stores++;
    store->n_timestamp_store = 0;
    if(store->n_stores >= store->size_stores){
      unsigned int i;
      unsigned int new_size_stores = store->size_stores * 2;
      store->stores = realloc_tester(store->stores, new_size_stores * sizeof(timestamp_linked_list*));
      for(i = store->size_stores; i < new_size_stores; i++)
	store->stores[i] = calloc_tester((store->size_timestamp_store + 1), sizeof(timestamp_linked_list));
    }
  }
  retval =  & store->stores[store->n_stores][store->n_timestamp_store];
#ifdef HAVE_PTHREAD
  pthread_mutex_unlock(& store->lock);
#endif
  return retval;
}

/**
   Adds a single timestamp.

   Called from reamining_axiom_false_in_factset in rete_state.c

   Necessary for the output of correct coq proofs
**/
void add_timestamp(timestamps* ts, timestamp t, timestamp_store* store){  
  timestamp_linked_list* new_ts = get_timestamp_memory(store);
#ifndef NDEBUG
  unsigned int orig_ts_len = get_n_timestamps(ts);
#endif
  printf("Adds timestamp %i\n", t.step);
  new_ts->ts = t;
  new_ts->next = ts->list;
  ts->list = new_ts;
  if(ts->first == NULL){
    assert(ts->list->next == NULL);
    ts->first = new_ts;
  }
  assert(orig_ts_len + 1 == get_n_timestamps(ts));
}

/**
   Adds the timestamps in orig to those in dest.
   Called from union_substitutions_struct_with_ts in substitution.c
**/
#define __COSTLY_TIMESTAMP_UNON

#ifdef __COSTLY_TIMESTAMP_UNON
void add_timestamps(timestamps* dest, const timestamps* orig, timestamp_store* store){
#ifndef NDEBUG
  unsigned int n_dest = get_n_timestamps(dest);
  unsigned int n_orig = get_n_timestamps(orig);
  unsigned int n_new;
#endif
  printf("Union of timestamps.\n");
  timestamp_linked_list* orig_el = orig->list;
  while(orig_el != NULL){
    if(orig_el->ts.step != dest->first->ts.step)
      add_timestamp(dest, orig_el->ts, store);
#ifndef NDEBUG
    else {
      printf("Same timestamp %u added.\n", orig_el->ts.step);
      n_orig--;
    }
#endif
    orig_el = orig_el->next;
  }
#ifndef NDEBUG
  n_new = get_n_timestamps(dest);
  assert(n_dest + n_orig == n_new);
#endif
}

#else

void add_timestamps(timestamps* dest, const timestamps* orig, timestamp_store* store){
#ifndef NDEBUG
  unsigned int n_dest = get_n_timestamps(dest);
  unsigned int n_orig = get_n_timestamps(orig);
  unsigned int n_new;
#endif
  if(dest->first == NULL){
    assert(dest->list == NULL);
    dest->first = orig->first;
    dest->list = orig->list;
  } else if(dest->first != orig->list){
    #ifndef NDEBUG
    timestamp_linked_list * el = orig->list;
    while(el != NULL){
      assert(el != dest->first);
      el = el->next;
    }
    #endif
    dest->first->next = orig->list;
#ifndef NDEBUG
    n_new = get_n_timestamps(dest);
    assert(n_dest + n_orig == n_new);
#endif
  }
}
#endif

/**
   Internal function for comparing timestamps on a substition

   They correspond to the times at which the matching for each conjunct
   was introduced to the factset

   Returns positive if first is larger(newer) than last, negative if first is smaller(newer) than last,
   and 0 if they are equal
**/
int compare_timestamps(const timestamps* first, const timestamps* last){
  timestamp_linked_list *first_el, *last_el;
  assert(first_el != NULL || last_el == NULL);
  while(first_el != NULL){
    assert(last_el != NULL);
    if(first_el->ts.step != last_el->ts.step)
      return first_el->ts.step - last_el->ts.step;
  }
  assert(last_el == NULL);
  return 0;
}

/**
   Discovers how much space is needed for the timestmaps, substitutions and rule instances

   This is declared in substitution_size_info.h
**/
substitution_size_info init_sub_size_info(unsigned int n_vars, unsigned int max_lhs_conjuncts){
  substitution_size_info ssi;
  unsigned int size_vars, size_timestamps;
  ssi.max_n_timestamps = max_lhs_conjuncts + 1;
  size_vars = n_vars * sizeof(term*);
  size_timestamps = 0;
  ssi.size_substitution = sizeof(substitution) + size_vars;
  ssi.size_rule_instance = sizeof(rule_instance) + size_vars;
  ssi.sub_values_offset =  0;
  return ssi;
}


/**
   Called from rete_state_single. 
   
   timestamps are in a double array, size_timestamp_store must never be changed after initializing,
   while size_stores is changed whenever more memory is needed
**/
timestamp_store* init_timestamp_store(substitution_size_info ssi){
  timestamp_store* ts = malloc_tester(sizeof(timestamp_store));
#ifdef HAVE_PTHREAD
  pthread_mutexattr_t mutex_attr;
  pt_err(pthread_mutexattr_init(&mutex_attr), "rule_queue_single.c: initialize_queue_single: mutex attr init");
#ifndef NDEBUG
  pt_err(pthread_mutexattr_settype(&mutex_attr, PTHREAD_MUTEX_ERRORCHECK_NP),  "rule_queue_single.c: initialize_queue_single: mutex attr settype");
#endif
  pt_err(pthread_mutex_init(& ts->lock, &mutex_attr), "timestamp_linked_list.c: init_timestamp_store: mutex init.\n");
#endif
  ts->size_timestamp_store = 1000;
  ts->n_timestamp_store = 0;
  ts->size_stores = 1;
  ts->n_stores = 0;
  ts->stores = calloc_tester(ts->size_stores, sizeof(timestamp_linked_list*));
  ts->stores[0] = calloc_tester(ts->size_timestamp_store + 1, sizeof(timestamp_linked_list));
  return ts;
}

void destroy_timestamp_store(timestamp_store* store){
  unsigned int i;
#ifdef HAVE_PTHREAD
  pt_err(pthread_mutex_destroy(& store->lock), "#__FILE__: #__LINE__: mutexdestroy");;
#endif
  for(i = 0; i < store->size_stores; i++)
    free(store->stores[i]);
  free(store->stores);
  free(store);
}

timestamp_store_backup backup_timestamp_store(timestamp_store* ts){
  timestamp_store_backup b;
  b.store = ts;
  b.n_timestamp_store = ts->n_timestamp_store;
  b.n_stores = ts->n_stores;
  return b;
}
timestamp_store* restore_timestamp_store(timestamp_store_backup b){
  b.store->n_timestamp_store = b.n_timestamp_store;
  b.store->n_stores = b.n_stores;
  return b.store;
}
