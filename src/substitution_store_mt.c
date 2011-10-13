/* substitution_store_mt.c

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
#include "substitution_store_mt.h"
#include "rete_net.h"

#ifdef HAVE_PTHREAD
#include <pthread.h>
#endif


void check_non_zero_error_code(int rv, char* msg){
  if(rv != 0){
    fprintf(stderr, "%s line %i %s: %s\n", __FILE__, __LINE__, msg, strerror(rv));
    assert(false);
    exit(EXIT_FAILURE);
  }
}

/**
   Error-checking functions for locking and unlocking the mutex
**/
void lock_sub_store_mutex(char* err_str, substitution_store_mt* sm){
#ifdef HAVE_PTHREAD
  int rv = pthread_mutex_lock(& sm->sub_store_mutex);
  if(rv != 0){
    fprintf(stderr, "%s substitution_store_mt.c: line %i: Cannot lock mutex: %s.\n", err_str, __LINE__, strerror(rv));
    assert(false);
    exit(EXIT_FAILURE);
  }
#endif
}

void unlock_sub_store_mutex(char* err_str, substitution_store_mt* sm){
#ifdef HAVE_PTHREAD
  int rv = pthread_mutex_unlock(& sm->sub_store_mutex);
  if(rv != 0){
    fprintf(stderr, "%s substitution_store_mt.c: line %i : Cannot unlock mutex: %s.\n", err_str, __LINE__, strerror(rv));
    assert(false);
    exit(EXIT_FAILURE);
  }
#endif
}

void wait_sub_store(char* err_str,substitution_store_mt* sm){
#ifdef HAVE_PTHREAD
  int rv = pthread_cond_wait(&sm->sub_store_cond, & sm->sub_store_mutex);
  if(rv != 0){
    fprintf(stderr, "%s substitution_store_mt.c: line %i: wait_sub_store(): Cannot wait on pthread_cond %s.\n", err_str, __LINE__, strerror(rv));
    assert(false);
    exit(EXIT_FAILURE);
  }
#endif
}


void broadcast_sub_store(char* err_str, substitution_store_mt* sm){
#ifdef HAVE_PTHREAD
  int rv = pthread_cond_broadcast(&sm->sub_store_cond);
  if(rv != 0){
    fprintf(stderr, "%s substitution_store_mt.c: line %i: wait_sub_store(): Cannot wait on pthread_cond %s.\n", err_str, __LINE__, strerror(rv));
    assert(false);
    exit(EXIT_FAILURE);
  }
#endif
}


/**
   Thread-safe function for getting a unique next index into the substitution_store
**/
size_t get_sub_store_index(size_t store_index, substitution_store_mt* sm){
  size_t subst_index;
#ifdef __GNUC__
  return __sync_fetch_and_add(&(sm->n_cur_substitution_store[store_index]), 1);
#else
  lock_sub_store_mutex("get_sub_store_index", sm);
  subst_index = sm->n_cur_substitution_store[store_index];
  sm->n_cur_substitution_store[store_index]++;
  unlock_sub_store_mutex("get_sub_store_index", sm);
#endif
  return subst_index;
}

/**
   Creates a new store for substitutions.
   This function is threadsafe, but this should not be an issue, 
   as get_substitution_store_mt should only call it once for every 
   "size_substitution_store" number of calls
**/
void new_substitution_store(size_t store_index, substitution_store_mt* sm, substitution_size_info ssi){
  size_t new_store_index;
  assert(sm->n_substitution_stores == 0 || sm->n_cur_substitution_store[store_index] >= sm->size_substitution_store);
  lock_sub_store_mutex("new_substitution_store()", sm);
  if(sm->n_substitution_stores > 0 && store_index < sm->n_substitution_stores - 1){
    fprintf(stderr, "substitution_store_mt.c: Overlapping calls to new_substitution_store. Consider increasing the value of size_substitution_store.\n");
    unlock_sub_store_mutex("new_substitution_store()", sm);
    return;
  }
  new_store_index = sm->n_substitution_stores;
  assert(store_index == 0 || store_index == new_store_index - 1);
  if(new_store_index >= sm->size_substitution_stores){
    sm->size_substitution_stores *= 2;
    sm->substitution_stores = realloc_tester(sm->substitution_stores, sm->size_substitution_stores * sizeof(char*));
    sm->n_cur_substitution_store = realloc_tester(sm->n_cur_substitution_store, sm->size_substitution_stores * sizeof(size_t));
  }
  assert(new_store_index < sm->size_substitution_stores);

  sm->substitution_stores[new_store_index] = malloc_tester(sm->size_substitution_store * get_size_substitution(ssi));
  sm->n_cur_substitution_store[new_store_index] = 0;  
  sm->n_substitution_stores++;
#ifdef HAVE_PTHREAD
  broadcast_sub_store("new_substitution_store()", sm);
  unlock_sub_store_mutex("new_substitution_store()", sm);
#endif
}

substitution_store_mt init_substitution_store_mt(substitution_size_info ssi){
  substitution_store_mt new_sm;
  int rv;
#ifdef HAVE_PTHREAD  
  pthread_mutexattr_t mutexattr;
#endif

  new_sm.size_substitution_stores = 10;
  new_sm.size_substitution_store = INIT_SUBST_MEM_SIZE;
  new_sm.substitution_stores = calloc(new_sm.size_substitution_stores, sizeof(char*));
  new_sm.n_cur_substitution_store = calloc(new_sm.size_substitution_stores, sizeof(size_t));
  new_sm.n_substitution_stores = 0;

#ifdef HAVE_PTHREAD
  pthread_mutexattr_init(&mutexattr);
#ifndef NDEBUG
  rv = pthread_mutexattr_settype(&mutexattr, PTHREAD_MUTEX_ERRORCHECK_NP);
  check_non_zero_error_code(rv, "Could not set mutex type");
#endif
  rv = pthread_mutex_init(& new_sm.sub_store_mutex, &mutexattr); 
  check_non_zero_error_code(rv, "Could not initialize mutex");
  pthread_mutexattr_destroy(&mutexattr);
  rv = pthread_cond_init(& new_sm.sub_store_cond, NULL);
  check_non_zero_error_code(rv, "Could not initialize condition variable");
#endif

  new_substitution_store(0, &new_sm, ssi);
  return new_sm;
}



/**
   Thread-safe function for getting memory for a substitution

   Assumes that init_substitution_store_mt has been called first.
**/
substitution* get_substitution_store_mt(substitution_store_mt* sm, substitution_size_info ssi){
  size_t subst_index, store_index;
  substitution* new_subst;
  char* memory_start;
  assert(get_size_substitution(ssi) > 0 && sm->substitution_stores != NULL && sm->n_substitution_stores > 0);
  store_index = sm->n_substitution_stores - 1;
  subst_index = get_sub_store_index(store_index, sm);
  if(subst_index >= sm->size_substitution_store){
    if(subst_index == sm->size_substitution_store){
      new_substitution_store(store_index, sm, ssi);
    } else { // subst_index > size_substitution_store
#ifdef HAVE_PTHREAD
      lock_sub_store_mutex("get_substitution_store_mt()", sm);
      while(sm->n_cur_substitution_store[sm->n_substitution_stores - 1] > sm->size_substitution_store)
	wait_sub_store("get_substitution_store_mt()", sm);
      unlock_sub_store_mutex("get_substitution_store_mt()", sm);
#else
      assert(false);
#endif
    }
    return get_substitution_store_mt(sm, ssi);
  }
  memory_start =  sm->substitution_stores[store_index] + subst_index * get_size_substitution(ssi);
  new_subst = (substitution*) memory_start;
  return new_subst;
}

/**
   Deletes all substitutions gotten previously through get_substitution_store_mt
**/
void destroy_substitution_store_mt(substitution_store_mt* sm){
  unsigned int i;
  int rv;
  for(i = 0; i < sm->n_substitution_stores; i++)
    free(sm->substitution_stores[i]);
  free(sm->substitution_stores);
#ifdef HAVE_PTHREAD
  rv = pthread_mutex_destroy(& sm->sub_store_mutex); 
  check_non_zero_error_code(rv, "Could not destroy mutex");
  rv = pthread_cond_destroy(& sm->sub_store_cond);
  check_non_zero_error_code(rv, "Could not destroy condition variable");
#endif
}
