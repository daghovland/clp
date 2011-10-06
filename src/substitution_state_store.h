/* substitution_state_store.h

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

#ifndef __INCLUDE_SUBSTITUTION_STATE_STORE_H
#define __INCLUDE_SUBSTITUTION_STATE_STORE_H

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

typedef struct substitution_store_t {
  char* store;
  size_t size_store;
  unsigned int max_n_subst;
  unsigned int n_subst;
} substitution_store;



substitution_store init_state_subst_store(substitution_size_info);
void destroy_state_subst_store(substitution_store*);
unsigned int alloc_substitution(substitution_store*, substitution_size_info);
substitution* get_substitution(unsigned int, substitution_store*, substitution_size_info);
#endif
