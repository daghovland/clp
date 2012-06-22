/* substitution_store_array.c

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
#include "substitution_store_array.h"



/**
   Auxiliary function for create_rete_state_single
**/
substitution_store_array* init_substitution_store_array(substitution_size_info ssi, unsigned int n_stores){
  unsigned int i;
  substitution_store_array * stores = malloc_tester(sizeof(substitution_store_array));
  stores->stores = calloc_tester(n_stores, sizeof(substitution_store));
  stores->n_stores = n_stores;
  for(i = 0; i < stores->n_stores; i++)
    stores->stores[i] = init_substitution_store(ssi);
  return stores;
}



void destroy_substitution_store_array(substitution_store_array* stores){
  unsigned int i;
  for(i = 0; i < stores->n_stores; i++)
    destroy_substitution_store(&stores->stores[i]);
  free(stores->stores);
  free(stores);
}

substitution_store * get_substitution_store(substitution_store_array* stores, unsigned int no){
  return & stores->stores[no];
}


sub_store_iter get_array_sub_store_iter(substitution_store_array* stores, unsigned int node_no){
  return get_sub_store_iter(get_substitution_store(stores, node_no));
}


/**
   Insert a copy of substition into the state, if not already there (modulo relevant_vars)
   Used when inserting into alpha- and beta-stores and rule-stores.
   
   Note that this functions is not thread-safe, since substitution_store is not

   The substitution a is not changed or freed. THe calling function must free it after the call returns

**/
bool insert_substitution_single(substitution_store_array* stores, unsigned int sub_no, const substitution* a, const freevars* relevant_vars, constants* cs, timestamp_store* ts_store){
  sub_store_iter iter = get_array_sub_store_iter(stores, sub_no);
  
  while(has_next_sub_store(&iter)){
    substitution* next_sub = get_next_sub_store(&iter);
    if(literally_equal_substitutions(a, next_sub, relevant_vars, cs))
      return false;
  }
  destroy_sub_store_iter(&iter);
  push_substitution_sub_store(get_substitution_store(stores, sub_no), a, ts_store, cs);
  return true;
}


/**
   Auxiliaries for manipulating array of substitution store backups
**/

substitution_store_array_backup * backup_substitution_store_array(substitution_store_array* stores){
  unsigned int i;
  substitution_store_array_backup * backup = malloc(sizeof(substitution_store_array_backup)) ;
  backup->backups = calloc_tester(stores->n_stores, sizeof(substitution_store_backup)); 
  backup->stores = stores;
  for(i = 0; i < stores->n_stores; i++)
    backup->backups[i] = backup_substitution_store(& stores->stores[i]);
  return backup;
}

substitution_store_array * restore_substitution_store_array(substitution_store_array_backup* backup){
  unsigned int i;
  substitution_store_array* stores = backup->stores;
  for(i = 0; i < stores->n_stores; i++)
    restore_substitution_store(& stores->stores[i], backup->backups[i]);
  return stores;
}

void destroy_substitution_store_array_backup(substitution_store_array_backup* backups){
  unsigned int i;
  for(i = 0; i < backups->stores->n_stores; i++)
    destroy_substitution_backup(& backups->backups[i]);
  free(backups->backups);
  free(backups);
}
