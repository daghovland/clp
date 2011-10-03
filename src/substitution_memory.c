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
   Threadsafety assured
**/
#include "common.h"
#include "term.h"
#include "theory.h"
#include "substitution_memory.h"

substitution** substitution_stores = NULL;
size_t size_substitution_store;
size_t size_substitution = 0;
size_t n_cur_substitution_store;
size_t n_substitution_stores;
size_t size_substitution_stores;

void init_substitution_memory(const theory* t){
  size_substitution = sizeof(substitution) + t->vars->n_vars * sizeof(term*);
  size_substitution_store = 1000000;
  size_substitution_stores = 10;
  substitution_stores = calloc(size_substitution_stores, sizeof(substitution*));
  n_substitution_stores = 0;
}

void new_substitution_store(){
  n_substitution_stores++;
  if(n_substitution_stores >= size_substitution_stores){
    size_substitution_stores *= 2;
    substitution_stores = realloc(substitution_stores, size_substitution_stores);
  }
  substitution_stores[n_substitution_stores] = malloc(size_substitution_store * size_substitution);
  n_cur_substitution_store = 0;  
}

substitution* get_substitution_memory(){
  assert(size_substitution > 0 && substitution_stores != NULL);
  n_cur_substitution_store++;
  if(n_cur_substitution_store >= size_substitution_store)
    new_substitution_store();
  return substitution_stores[n_substitution_stores-1] + n_cur_substitution_store - 1;
}

void destroy_substitution_memory(){
  unsigned int i;
  for(i = 0; i < n_substitution_stores; i++)
    free(substitution_stores[i]);
  free(substitution_stores);
}

size_t get_size_substitution(){
  return size_substitution;
}
