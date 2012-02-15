/* timestamp_store.c

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
#include "timestamps.h"
#include "term.h"
#include "substitution.h"
#include "rule_instance.h"
#include "error_handling.h"
#ifdef HAVE_PTHREAD
#include <pthread.h>
#endif
#ifndef USE_TIMESTAMP_ARRAY


timestamp_linked_list* get_timestamp_memory(timestamp_store* store, bool permanent){
  timestamp_linked_list* retval;
  if(permanent)
    return malloc_tester(sizeof(timestamp_linked_list));
#ifdef HAVE_PTHREAD
  pt_err(pthread_mutex_lock(& store->lock), __FILE__, __LINE__, ": mutex lock");
#endif
#ifdef USE_TIMESTAMP_STORE_ARRAY
  if(store->n_timestamp_store >= store->size_timestamp_store){
    store->n_stores++;
    store->n_timestamp_store = 0;
    while(store->n_stores >= store->size_stores){
      unsigned int i;
      unsigned int new_size_stores = store->size_stores * 2;
      store->stores = realloc_tester(store->stores, new_size_stores * sizeof(timestamp_linked_list*));
      for(i = store->size_stores; i < new_size_stores; i++)
	store->stores[i] = calloc_tester((store->size_timestamp_store + 1), sizeof(timestamp_linked_list));
      store->size_stores = new_size_stores;
    }
    assert(store->n_stores < store->size_stores);
  }
  retval =  & store->stores[store->n_stores][store->n_timestamp_store];
  assert(retval != NULL);
  store->n_timestamp_store++;
#else
  retval = malloc_tester(sizeof(timestamp_linked_list));
  if(store->n_timestamp_store >= store->size_timestamp_store){
    store->size_timestamp_store *= 2;
    store->stores = realloc(store->stores, sizeof(timestamp_linked_list*) * store->size_timestamp_store);
  }
  store->stores[store->n_timestamp_store] = retval;
  store->n_timestamp_store++;
#endif
#ifdef HAVE_PTHREAD
  pthread_mutex_unlock(& store->lock);
#endif
  assert(retval != NULL);
  return retval;
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
  pt_err(pthread_mutexattr_init(&mutex_attr), __FILE__, __LINE__, "rule_queue_single.c: initialize_queue_single: mutex attr init");
#ifndef NDEBUG
  pt_err(pthread_mutexattr_settype(&mutex_attr, PTHREAD_MUTEX_ERRORCHECK_NP), __FILE__, __LINE__,  "rule_queue_single.c: initialize_queue_single: mutex attr settype");
#endif
  pt_err(pthread_mutex_init(& ts->lock, &mutex_attr), __FILE__, __LINE__, "timestamp_linked_list.c: init_timestamp_store: mutex init.\n");
#endif
  ts->size_timestamp_store = 1;
  ts->n_timestamp_store = 0;
#ifdef USE_TIMESTAMP_STORE_ARRAY
  ts->size_stores = 1;
  ts->n_stores = 0;
  ts->stores = malloc_tester(ts->size_stores * sizeof(timestamp_linked_list*));
  ts->stores[0] = calloc_tester(ts->size_timestamp_store + 1, sizeof(timestamp_linked_list));
#else
  ts->stores = calloc_tester(ts->size_timestamp_store + 1, sizeof(timestamp_linked_list*));
#endif
  return ts;
}

#ifndef USE_TIMESTAMP_STORE_ARRAY
void backtrack_timestamp_store(timestamp_store* store, unsigned int limit){
  unsigned int i;
  for(i = store->n_timestamp_store; i > limit; i--){
    free(store->stores[store->n_timestamp_store]);
  }
  store->n_timestamp_store = limit;
}
#endif

void destroy_timestamp_store(timestamp_store* store){
#ifdef USE_TIMESTAMP_STORE_ARRAY
  unsigned int i;
  for(i = 0; i < store->size_stores; i++)
    free(store->stores[i]);
#else
  backtrack_timestamp_store(store, 0);
#endif
#ifdef HAVE_PTHREAD
  pt_err(pthread_mutex_destroy(& store->lock), __FILE__, __LINE__, ": mutexdestroy");;
#endif
  free(store->stores);
  free(store);
}

timestamp_store_backup backup_timestamp_store(timestamp_store* ts){
  timestamp_store_backup b;
  b.store = ts;
  b.n_timestamp_store = ts->n_timestamp_store;
#ifdef USE_TIMESTAMP_STORE_ARRAY
  b.n_stores = ts->n_stores;
#endif  
  return b;

}
timestamp_store* restore_timestamp_store(timestamp_store_backup b){
#ifdef USE_TIMESTAMP_STORE_ARRAY
  b.store->n_timestamp_store = b.n_timestamp_store;
  b.store->n_stores = b.n_stores;
#else
  backtrack_timestamp_store(b.store, b.n_timestamp_store);
#endif
  return b.store;
}

#endif
