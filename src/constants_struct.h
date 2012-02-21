/* constants_struct.h

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
   Used by parser and the rete states to keep track of constants
**/

#ifndef __INCLUDE_CONSTANTS_STRUCT_H
#define __INCLUDE_CONSTANTS_STRUCT_H

#include "common.h"
#include "fresh_constants.h"
#include "timestamps.h"
#ifdef HAVE_PTHREAD
#include <pthread.h>
#endif

/**
   Part of a union-find / disjoint set structure

   http://en.wikipedia.org/wiki/Disjoint-set_data_structure 
**/
typedef struct dom_elem_t {
  char* name;
  unsigned int id;
} dom_elem;


typedef struct constant_t {
  dom_elem elem;
  timestamps steps;
  unsigned int parent;
  unsigned int rank;
  timestamps ts;
} constant;

/**
   Used by the rete state to keep track of the constants
**/
typedef struct constants_t {
  fresh_const_counter fresh;
  constant* constants;
  size_t size_constants;
#ifdef HAVE_PTHREAD
  pthread_mutex_t constants_mutex;
#endif
  unsigned int n_constants;
} constants;

typedef unsigned int constants_iter ;

#endif
