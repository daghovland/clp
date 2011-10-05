/* substitution_state_store.c

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
#include "substitution_state_store.h"


substitution_store init_state_subst_store(void){
  substitution_store new_store;
  new_store.max_n_subst = INIT_SUBST_STORE_SIZE;
  new_store.size_store = new_store.max_n_subst * get_size_full_substitution();
  new_store.n_subst = 0;
  new_store.store = malloc_tester(new_store.size_store);
  return new_store;
}

void destroy_state_subst_store(substitution_store* store){
  free(store->store);
}

unsigned int alloc_substitution(substitution_store* store){
  char* new_ptr;
  unsigned int new_i = store->n_subst;
  store->n_subst ++;
  if(store->n_subst >= store->max_n_subst){
    store->max_n_subst *= 2;
    store->size_store *= 2;
    store->store = realloc_tester(store->store, store->size_store);
  }
  new_ptr = store->store + (new_i * get_size_full_substitution());
  ((substitution*) new_ptr)->timestamps =  (signed int*) (new_ptr + get_size_substitution() + get_substitution_timestamp_offset());
  return store->n_subst - 1;
}


substitution* get_substitution(unsigned int i, substitution_store* store){
  assert(i < store->max_n_subst);
  return (substitution*) (store->store + (i * get_size_full_substitution()));
}
