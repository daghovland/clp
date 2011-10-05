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
#include "theory.h"
#include "substitution_memory.h"

#ifdef HAVE_PTHREAD
#include <pthread.h>
pthread_mutex_t sub_store_mutex = 
#ifndef NDEBUG
  PTHREAD_ERRORCHECK_MUTEX_INITIALIZER_NP;
#else
  PTHREAD_MUTEX_INITIALIZER;
#endif
pthread_cond_t sub_store_cond = PTHREAD_COND_INITIALIZER;
#endif




char** substitution_stores = NULL;
size_t size_substitution_store = 10000;
size_t size_substitution = 0;
size_t size_full_substitution = 0;
size_t size_timestamps;
size_t * n_cur_substitution_store;
size_t n_substitution_stores = 0;
size_t size_substitution_stores = 10;
int substitution_timestamp_offset;

/**
   Error-checking functions for locking and unlocking the mutex
**/
void lock_sub_store_mutex(char* err_str){
#ifdef HAVE_PTHREAD
  int rv = pthread_mutex_lock(& sub_store_mutex);
  if(rv != 0){
    fprintf(stderr, "%s substitution_memory.c: get_substitution_memory(): Cannot lock mutex: %s.\n", err_str, strerror(rv));
    exit(EXIT_FAILURE);
  }
#endif
}

void unlock_sub_store_mutex(char* err_str){
#ifdef HAVE_PTHREAD
  int rv = pthread_mutex_unlock(& sub_store_mutex);
  if(rv != 0){
    fprintf(stderr, "%s substitution_memory.c: : Cannot unlock mutex: %s.\n", err_str, strerror(rv));
    exit(EXIT_FAILURE);
  }
#endif
}

void wait_sub_store(char* err_str){
#ifdef HAVE_PTHREAD
  int rv = pthread_cond_wait(&sub_store_cond, & sub_store_mutex);
  if(rv != 0){
    fprintf(stderr, "%s substitution_memory.c: wait_sub_store(): Cannot wait on pthread_cond %s.\n", err_str, strerror(rv));
    exit(EXIT_FAILURE);
  }
#endif
}


void broadcast_sub_store(char* err_str){
#ifdef HAVE_PTHREAD
  int rv = pthread_cond_broadcast(&sub_store_cond);
  if(rv != 0){
    fprintf(stderr, "%s substitution_memory.c: wait_sub_store(): Cannot wait on pthread_cond %s.\n", err_str, strerror(rv));
    exit(EXIT_FAILURE);
  }
#endif
}


/**
   Thread-safe function for getting a unique next index into the substitution_store
**/
size_t get_sub_store_index(size_t store_index){
  size_t subst_index;
#ifdef __GNUC__
  return __sync_fetch_and_add(&(n_cur_substitution_store[store_index]), 1);
#else
  lock_sub_store_mutex("get_sub_store_index");
  subst_index = n_cur_substitution_store[store_index];
  n_cur_substitution_store[store_index]++;
  unlock_sub_store_mutex("get_sub_store_index");
#endif
  return subst_index;
}

/**
   Creates a new store for substitutions.
   This function is threadsafe, but this should not be an issue, 
   as get_substitution_memory should only call it once for every 
   "size_substitution_store" number of calls
**/
void new_substitution_store(size_t store_index){
  size_t new_store_index;
  assert(n_substitution_stores == 0 || n_cur_substitution_store[store_index] >= size_substitution_store);
  lock_sub_store_mutex("new_substitution_store()");
  if(n_substitution_stores > 0 && store_index < n_substitution_stores - 1){
    fprintf(stderr, "substitution_memory.c: Overlapping calls to new_substitution_store. Consider increasing the value of size_substitution_store.\n");
    unlock_sub_store_mutex("new_substitution_store()");
    return;
  }
  new_store_index = n_substitution_stores;
  assert(store_index == 0 || store_index == new_store_index - 1);
  if(new_store_index >= size_substitution_stores){
    size_substitution_stores *= 2;
    substitution_stores = realloc_tester(substitution_stores, size_substitution_stores * sizeof(char*));
    n_cur_substitution_store = realloc_tester(n_cur_substitution_store, size_substitution_stores * sizeof(size_t));
  }
  assert(new_store_index < size_substitution_stores);

  substitution_stores[new_store_index] = malloc_tester(size_substitution_store * size_full_substitution);
  n_cur_substitution_store[new_store_index] = 0;  
  n_substitution_stores++;
#ifdef HAVE_PTHREAD
  broadcast_sub_store("new_substitution_store()");
  unlock_sub_store_mutex("new_substitution_store()");
#endif
}

void init_substitution_memory(const theory* t){
  size_substitution = sizeof(substitution) + (t->vars->n_vars) * sizeof(term*) ;
  size_timestamps = t->max_lhs_conjuncts;
  substitution_timestamp_offset = size_substitution % sizeof(signed int);
  if(substitution_timestamp_offset != 0)
    substitution_timestamp_offset = sizeof(signed int) - substitution_timestamp_offset;
  assert(substitution_timestamp_offset >= 0 && substitution_timestamp_offset < sizeof(signed int));
  size_full_substitution = size_substitution + size_timestamps * sizeof(signed int) + substitution_timestamp_offset;
  substitution_stores = calloc(size_substitution_stores, sizeof(char*));
  n_cur_substitution_store = calloc(size_substitution_stores, sizeof(size_t));
  n_substitution_stores = 0;
  new_substitution_store(0);
}



/**
   Thread-safe function for getting memory for a substitution

   Assumes that init_substitution_memory has been called first.
**/
substitution* get_substitution_memory(){
  size_t subst_index, store_index;
  substitution* new_subst;
  char* memory_start;
  assert(size_substitution > 0 && substitution_stores != NULL && n_substitution_stores > 0);
  store_index = n_substitution_stores - 1;
  subst_index = get_sub_store_index(store_index);
  if(subst_index >= size_substitution_store){
    if(subst_index == size_substitution_store){
      new_substitution_store(store_index);
    } else { // subst_index > size_substitution_store
#ifdef HAVE_PTHREAD
      lock_sub_store_mutex("get_substitution_memory()");
      while(n_cur_substitution_store[n_substitution_stores - 1] > size_substitution_store)
	wait_sub_store("get_substitution_memory()");
      unlock_sub_store_mutex("get_substitution_memory()");
#else
      assert(false);
#endif
    }
    return get_substitution_memory();
  }
  memory_start =  substitution_stores[store_index] + subst_index * get_size_full_substitution();
  new_subst = (substitution*) memory_start;
  new_subst->timestamps = (signed int*) (memory_start + size_substitution + substitution_timestamp_offset);
  return new_subst;
}

/**
   Deletes all substitutions gotten previously through get_substitution_memory
**/
void destroy_substitution_memory(){
  unsigned int i;
  for(i = 0; i < n_substitution_stores; i++)
    free(substitution_stores[i]);
  free(substitution_stores);
}

size_t get_size_full_substitution(){
  return size_full_substitution;
}

size_t get_size_substitution(){
  return size_substitution;
}

size_t get_size_timestamps(){
  return size_timestamps;
}
