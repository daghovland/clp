/* substitution_struct.h

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

#ifndef __INCLUDE_SUBSTITUTION_STRUCT_H
#define __INCLUDE_SUBSTITUTION_STRUCT_H

#include "common.h"
#include "term.h"
#include "variable.h"
#include "timestamps.h"
/**
   An substitution of variables
   to term values

   The variables are those found in the freevars 
   set of the containing store
   
   Only "values" is used

   The pointer allvars points to the freevar of _all_ variables in the theory. 

   The timestamps are used to know when substitutions were inserted into the rete network.
   This is necessary for pruning and for constructing coq output proofs
   The timestamps are a list of the timestamps when each conjunct in the precedent was 
   matched by the factset. The length of timestamps is given by get_size_timestamps in
   substitution_memory, which is set by theory->max_lhs_conjuncts in init_substitution_memory

   sub_values_offset is the offset of the sub_values, caused by the dynamic array member in sub_ts. 
   This is fixed for each theory, since it depends on the number of premises of a clause.
   At the moment this is 0

   Recall that sizeof(substitution) is not correct because of 
**/
typedef struct substitution_t {
  unsigned int n_subs;
  const freevars* allvars;
  unsigned int sub_values_offset;
  timestamps sub_ts;
  const clp_term* sub_values[];
} substitution;

const clp_term* get_sub_value(const substitution*, unsigned int);
void set_sub_value(substitution*, unsigned int, const clp_term*);

#endif
