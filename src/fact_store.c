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
   Memory helper for facts, to avoid millions of calls to malloc

   These are not thread-safe, and used for the rete nodes 
**/
#include "common.h"
#include "fact_store.h"
#include "instantiate.h"


fact_store init_fact_store(){
  fact_store new_store;
  new_store.max_n_facts = INIT_FACT_STORE_SIZE;
  new_store.n_facts = 0;
  new_store.store = calloc_tester(new_store.max_n_facts, sizeof(clp_atom));
  return new_store;
}

void destroy_fact_store(fact_store* store){
#if false
  fact_store_iter fi = get_fact_store_iter(store);
  while(has_next_fact_store(&fi)){
    delete_instantiated_atom((clp_atom*) get_next_fact_store(&fi));
  }
  destroy_fact_store_iter(&fi);
#endif
  free(store->store);
}

unsigned int alloc_store_fact(fact_store* store){
  store->n_facts ++;
  if(store->n_facts >= store->max_n_facts){
    store->max_n_facts *= 2;
    store->store = realloc_tester(store->store, store->max_n_facts * sizeof(clp_atom));
  }
  return store->n_facts - 1;
}

/**
  Adds a copy of the fact and all its substructures to the store
**/
void push_fact_store(fact_store* store, const clp_atom* new_fact){
  unsigned int fact_no = alloc_store_fact(store);
  assert(new_fact->args->n_args == 0 || new_fact->args->args[0] != NULL);
  store->store[fact_no] = * new_fact;
}

const clp_atom* get_fact(unsigned int i, fact_store* store){
  assert(i < store->max_n_facts);
  return & store->store[i];
}

bool is_empty_fact_store(fact_store* store){
  return store->n_facts == 0;
}

fact_store_iter get_fact_store_iter(fact_store* store){
  fact_store_iter iter;
  iter.n = 0;
  iter.store = store;
  return iter;
}

bool has_next_fact_store(fact_store_iter* iter){
  return iter->n < iter->store->n_facts;
}

const clp_atom* get_next_fact_store(fact_store_iter* iter){
  const clp_atom* a = get_fact(iter->n, iter->store);
  assert(iter->n < iter->store->n_facts);
  iter->n ++;
  return a;
}


void destroy_fact_store_iter(fact_store_iter* iter){
  ;
}


/**
   Creates a "backup" of a fact store. Note that
   the store itself is not copied, just enough information to 
   backtrack. The store must not be deallocated before restoring.
**/
fact_store_backup backup_fact_store(fact_store* store){
  fact_store_backup backup;
  backup.store = store;
  backup.n_facts = store->n_facts;
  return backup;
}

void restore_fact_store(fact_store* store, fact_store_backup backup){
  assert(store == backup.store);
  store->n_facts = backup.n_facts;
}

void destroy_fact_backup(fact_store_backup * backup){
  ;
}

/**
   Copies a fact_store iter array in orig of length n_iters to 
   dest. Assumes the necessary memory in dest is already allocated
**/
void copy_fact_iter_array(fact_store_iter* dest, const fact_store_iter * orig, unsigned int n_iters){
  memcpy(dest, orig, n_iters * sizeof(fact_store_iter));
}


/**
   Auxiliaries for manipulating array of fact store backups
**/

fact_store_backup * backup_fact_store_array(fact_store* stores, unsigned int n_stores){
  unsigned int i;
  fact_store_backup* backups = calloc_tester(n_stores, sizeof(fact_store_backup));
  for(i = 0; i < n_stores; i++)
    backups[i] = backup_fact_store(& stores[i]);
  return backups;
}


void destroy_fact_store_backup_array(fact_store_backup* backups, unsigned int n_backups){
  unsigned int i;
  for(i = 0; i < n_backups; i++)
    destroy_fact_backup(& backups[i]);
  free(backups);
}



/**
   Prints all facts currently in the "factset"
**/
void print_fact_store(fact_store * store, const constants* cs, FILE* f){
  fact_store_iter iter = get_fact_store_iter(store);
  while(has_next_fact_store(&iter)){
    print_fol_atom(get_next_fact_store(&iter), cs, f);
    if(has_next_fact_store(&iter))
      fprintf(f, ", ");
  }
}
