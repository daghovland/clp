/* substitution_store.c

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

   These are not thread-safe, and used for the rete nodes 
**/
#include "common.h"
#include "term.h"
#include "theory.h"
#include "substitution_store.h"
#include "substitution_size_info.h"

substitution_store init_substitution_store(substitution_size_info ssi){
  substitution_store new_store;
  new_store.max_n_subst = INIT_SUBST_STORE_SIZE;
  new_store.n_subst = 0;
  new_store.ssi = ssi;
  new_store.store = calloc_tester(new_store.max_n_subst, get_size_substitution(ssi));
  return new_store;
}

void destroy_substitution_store(substitution_store* store){
  free(store->store);
}

unsigned int alloc_store_substitution(substitution_store* store){
  unsigned int new_i = store->n_subst;
  store->n_subst ++;
  if(store->n_subst >= store->max_n_subst){
    store->max_n_subst *= 2;
    store->store = realloc_tester(store->store, store->max_n_subst * get_size_substitution(store->ssi));
  }
  return new_i;
}

/**
  Adds a copy of the substitution and all its substructures to the store
**/
void push_substitution_sub_store(substitution_store* store, const substitution* new_sub, timestamp_store* ts_store, const constants* cs){
  unsigned int sub_no = alloc_store_substitution(store);
  copy_substitution_struct(get_substitution(sub_no, store), new_sub, store->ssi, ts_store, false, cs);
}

substitution* get_substitution(unsigned int i, substitution_store* store){
  assert(i < store->max_n_subst);
  return (substitution*) (store->store + (i * get_size_substitution(store->ssi)));
}


sub_store_iter get_sub_store_iter(substitution_store* store){
  sub_store_iter iter;
  iter.n = 0;
  iter.store = store;
  return iter;
}

bool has_next_sub_store(sub_store_iter* iter){
  return iter->n < iter->store->n_subst;
}

substitution* get_next_sub_store(sub_store_iter* iter){
  substitution* sub = get_substitution(iter->n, iter->store);
  assert(iter->n < iter->store->n_subst);
  iter->n ++;
  return sub;
}


void destroy_sub_store_iter(sub_store_iter* iter){
  ;
}


/**
   Creates a "backup" of a substitution store. Note that
   the store itself is not copied, just enough information to 
   backtrack. The store must not be deallocated before restoring.
**/
substitution_store_backup backup_substitution_store(substitution_store* store){
  substitution_store_backup backup;
  backup.store = store;
  backup.n_subst = store->n_subst;
  return backup;
}

void restore_substitution_store(substitution_store* store, substitution_store_backup backup){
  assert(store == backup.store);
  store->n_subst = backup.n_subst;
}

void destroy_substitution_backup(substitution_store_backup * backup){
  ;
}

