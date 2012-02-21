/* substitution_store.h

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

#ifndef __INCLUDED_SUBSTITUTION_STORE_H
#define __INCLUDED_SUBSTITUTION_STORE_H

#define INIT_SUBST_STORE_SIZE 1000

#include "substitution.h"
#include "substitution_size_info.h"

/**
   A store of substitutions for use in states

   Not thread-safe, since the assumption is that these are
   only accessed from a single rete state.
   
   Uses information calculated in substitution_memory.c about
   size needed for substitutions
**/

/**
   Implements a cache of substitutions in a alpha or beta node
**/


typedef struct substitution_store_t {
  char* store;
  unsigned int max_n_subst;
  unsigned int n_subst;
  substitution_size_info ssi;
} substitution_store;

typedef struct substitution_store_backup_t {
  substitution_store* store;
  unsigned int n_subst;
} substitution_store_backup;

typedef struct sub_store_iter_t {
  substitution_store* store;
  unsigned int n;
} sub_store_iter;

substitution_store init_substitution_store(substitution_size_info);
void destroy_substitution_store(substitution_store*);
unsigned int alloc_store_substitution(substitution_store*);
void push_substitution_sub_store(substitution_store*, const substitution*, timestamp_store*, const constants* cs);
substitution* get_substitution(unsigned int, substitution_store*);

sub_store_iter get_sub_store_iter(substitution_store*);
bool has_next_sub_store(sub_store_iter*);
substitution* get_next_sub_store(sub_store_iter*);
void destroy_sub_store_iter(sub_store_iter*);


substitution_store_backup backup_substitution_store(substitution_store*);
void restore_substitution_store(substitution_store*, substitution_store_backup);
void destroy_substitution_backup(substitution_store_backup*);

#endif
