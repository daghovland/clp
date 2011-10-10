/* constants.h

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

#ifndef __INCLUDE_CONSTANTS_H
#define __INCLUDE_CONSTANTS_H

#include "common.h"
#include "theory.h"
#include "fresh_constants.h"

/**
   Used by the rete state to keep track of the constants
**/
typedef struct constants_t {
  fresh_const_counter fresh;
  const char** constants;
  size_t size_constants;
  unsigned int n_constants;
} constants;

const char* parser_new_constant(theory*, const char*);
void print_coq_constants(const theory*,FILE* stream);
constants init_constants(size_t);
void destroy_constants(constants*);
constants copy_constants(constants*);
void insert_constant_name(constants* , const char*);
#endif
