/* substitution_memory.c

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
   Memory helper for substitutions, to avoid millions of calls to malloc

   The stores for substitutions are global, but this should not be a problem

   Threadsafety assured only if the size_substitution_store is large enough that 
   there will not be overlapping calls to new_substitution_store
**/
#include "common.h"
#include "term.h"
#include "substitution_memory.h"
#include "rete_net.h"

#ifdef HAVE_PTHREAD
#include <pthread.h>
#endif



/**
   Error-checking functions for locking and unlocking the mutex
**/
void lock_sub_store_mutex(char* err_str, substitution_memory* sm){
#ifdef HAVE_PTHREAD
  int rv = pthread_mutex_lock(& sm->sub_store_mutex);
  if(rv != 0){
    fprintf(stderr, "%s substitution_memory.c: get_substitution_memory(): Cannot lock mutex: %s.\n", err_str, strerror(rv));
    exit(EXIT_FAILURE);
  }
#endif
}

void unlock_sub_store_mutex(char* err_str, substitution_memory* sm){
#ifdef HAVE_PTHREAD
  int rv = pthread_mutex_unlock(& sm->sub_store_mutex);
  if(rv != 0){
    fprintf(stderr, "%s substitution_memory.c: : Cannot unlock mutex: %s.\n", err_str, strerror(rv));
    exit(EXIT_FAILURE);
  }
#endif
}

void wait_sub_store(char* err_str,substitution_memory* sm){
#ifdef HAVE_PTHREAD
  int rv = pthread_cond_wait(&sm->sub_store_cond, & sm->sub_store_mutex);
  if(rv != 0){
    fprintf(stderr, "%s substitution_memory.c: wait_sub_store(): Cannot wait on pthread_cond %s.\n", err_str, strerror(rv));
    exit(EXIT_FAILURE);
  }
#endif
}


void broadcast_sub_store(char* err_str, substitution_memory* sm){
#ifdef HAVE_PTHREAD
  int rv = pthread_cond_broadcast(&sm->sub_store_cond);
  if(rv != 0){
    fprintf(stderr, "%s substitution_memory.c: wait_sub_store(): Cannot wait on pthread_cond %s.\n", err_str, strerror(rv));
    exit(EXIT_FAILURE);
  }
#endif
}


/**
   Thread-safe function for getting a unique next index into the substitution_store
**/
size_t get_sub_store_index(size_t store_index, substitution_memory* sm){
  size_t subst_index;
#ifdef __GNUC__
  return __sync_fetch_and_add(&(sm->n_cur_substitution_store[sm->store_index]), 1);
#else
  lock_sub_store_mutex("get_sub_store_index", sm);
  subst_index = sm->n_cur_substitution_store[sm->store_index];
  sm->n_cur_substitution_store[sm->store_index]++;
  unlock_sub_store_mutex("get_sub_store_index", sm);
#endif
  return subst_index;
}

/**
   Creates a new store for substitutions.
   This function is threadsafe, but this should not be an issue, 
   as get_substitution_memory should only call it once for every 
   "size_substitution_store" number of calls
**/
void new_substitution_store(size_t store_index, substitution_memory* sm, const rete_net* net){
  size_t new_store_index;
  assert(sm->n_substitution_stores == 0 || sm->n_cur_substitution_store[sm->store_index] >= sm->size_substitution_store);
  lock_sub_store_mutex("new_substitution_store()", sm);
  if(sm->n_substitution_stores > 0 && sm->store_index < sm->n_substitution_stores - 1){
    fprintf(stderr, "substitution_memory.c: Overlapping calls to new_substitution_store. Consider increasing the value of size_substitution_store.\n");
    unlock_sub_store_mutex("new_substitution_store()", sm);
    return;
  }
  new_store_index = sm->n_substitution_stores;
  assert(sm->store_index == 0 || sm->store_index == sm->new_store_index - 1);
  if(new_store_index >= sm->size_substitution_stores){
    sm->size_substitution_stores *= 2;
    sm->substitution_stores = realloc_tester(sm->substitution_stores, sm->size_substitution_stores * sizeof(char*));
    sm->n_cur_substitution_store = realloc_tester(sm->n_cur_substitution_store, sm->size_substitution_stores * sizeof(size_t));
  }
  assert(sm->new_store_index < sm->size_substitution_stores);

  sm->substitution_stores[new_store_index] = malloc_tester(sm->size_substitution_store * net->size_full_substitution);
  sm->n_cur_substitution_store[new_store_index] = 0;  
  sm->n_substitution_stores++;
#ifdef HAVE_PTHREAD
  broadcast_sub_store("new_substitution_store()", sm);
  unlock_sub_store_mutex("new_substitution_store()", sm);
#endif
}

subsitution_memory init_substitution_memory(const rete_net* net){
  substitution_memory new_sm;
  new_sm.size_substitution_stores = 10;
  new_sm.size_substitution_store = INIT_SUBST_MEM_SIZE;
  new_sm.substitution_stores = calloc(new_sm.size_substitution_store, sizeof(char*));
  new_sm.n_cur_substitution_store = calloc(new_sm.size_substitution_stores, sizeof(size_t));
  new_sm.n_substitution_stores = 0;
  new_substitution_store(0, new_sm);
  return new_sm;
}



/**
   Thread-safe function for getting memory for a substitution

   Assumes that init_substitution_memory has been called first.
**/
substitution* get_substitution_memory(substitution_memory* sm, const rete_net* net){
  size_t subst_index, store_index;
  substitution* new_subst;
  char* memory_start;
  assert(net->size_substitution > 0 && sm->substitution_stores != NULL && sm->n_substitution_stores > 0);
  store_index = sm->n_substitution_stores - 1;
  subst_index = get_sub_store_index(sm->store_index, sm);
  if(subst_index >= sm->size_substitution_store){
    if(subst_index == sm->size_substitution_store){
      new_substitution_store(sm->store_index, sm);
    } else { // subst_index > size_substitution_store
#ifdef HAVE_PTHREAD
      lock_sub_store_mutex("get_substitution_memory()", sm);
      while(sm->n_cur_substitution_store[sm->n_substitution_stores - 1] > sm->size_substitution_store)
	wait_sub_store("get_substitution_memory()", sm);
      unlock_sub_store_mutex("get_substitution_memory()", sm);
#else
      assert(false);
#endif
    }
    return get_substitution_memory();
  }
  memory_start =  sm->substitution_stores[store_index] + subst_index * net->size_full_substitution;
  new_subst = (substitution*) memory_start;
  new_subst->timestamps = (signed int*) (memory_start + net->size_substitution + net->substitution_timestamp_offset);
  return new_subst;
}

/**
   Deletes all substitutions gotten previously through get_substitution_memory
**/
void destroy_substitution_memory(substitution_memory* sm){
  unsigned int i;
  for(i = 0; i < sm->n_substitution_stores; i++)
    free(sm->substitution_stores[i]);
  free(sm->substitution_stores);
}
