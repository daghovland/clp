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
#include "constants_struct.h"
#include "theory.h"
#include "timestamps.h"


dom_elem parser_new_constant(constants*, const char*);
void print_coq_constants(const constants*,FILE* stream);
const clp_term* get_fresh_constant(clp_variable*, constants*);
const char* get_constant_name(dom_elem, const constants*);
constants* init_constants(unsigned int);
void destroy_constants(constants*);
constants* copy_constants(const constants*, timestamp_store*);
constants* backup_constants(const constants*, timestamp_store*);
void print_all_constants(constants*, FILE*);
bool equal_constants_mt(dom_elem, dom_elem, constants*, timestamps*, timestamp_store*, bool);
void union_constants(dom_elem, dom_elem, constants*, unsigned int, timestamp_store*);

constants_iter get_constants_iter(constants*);
bool constants_iter_has_next(constants*, constants_iter*);
constant* constants_iter_get_next(constants*, constants_iter*);
void destroy_constants_iter(constants_iter);

bool test_constant(dom_elem , const constants*);

#endif
