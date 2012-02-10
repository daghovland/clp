/* substitution_store_array.h

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

#ifndef __INCLUDED_SUBSTITUTION_STORE_ARRAY_H
#define __INCLUDED_SUBSTITUTION_STORE_ARRAY_H

#include "substitution_store.h"
#include "constants_struct.h"
/**
   Implements a cache of substitutions in a alpha or beta node
**/


typedef struct substitution_store_array_t {
  unsigned int n_stores;
  substitution_store * stores;
} substitution_store_array;

typedef struct substitution_store_array_backup_t {
  substitution_store_array* stores;
  substitution_store_backup * backups;
} substitution_store_array_backup;


substitution_store_array * init_substitution_store_array(substitution_size_info, unsigned int n_stores);
void destroy_substitution_store_array(substitution_store_array*);

substitution_store * get_substitution_store(substitution_store_array*, unsigned int);
sub_store_iter get_array_sub_store_iter(substitution_store_array*, unsigned int);
bool insert_substitution_single(substitution_store_array* stores, unsigned int sub_no, const substitution* a, const freevars* relevant_vars, constants*, timestamp_store*);

substitution_store_array* restore_substitution_store_array(substitution_store_array_backup*);
substitution_store_array_backup * backup_substitution_store_array(substitution_store_array*);
void destroy_substitution_store_array_backup(substitution_store_array_backup*);
#endif
