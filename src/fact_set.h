/* fact_set.h

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

#ifndef __INCLUDED_FACT_SET_H
#define __INCLUDED_FACT_SET_H

#include "substitution.h"
/**
   Representation of the fact set as a linked list of atoms

   Note that each fact-set is for a specific predicate, so it is unnecessary
   to have the whole atom. It would suffice with an array of constants

   prev is used during top-down iteration and is different in different states.
   The corresponding mutex must be locked before this can be changed or read.

   The timestamp is necessary for output of coq proofs.
   This should be the step at which the fact is inserted into the set.
**/
typedef struct fact_set_t {
  struct fact_set_t * next;
  struct fact_set_t * prev;
  unsigned int timestamp;
  bool split_point;
  const clp_atom* fact;
} fact_set;

void delete_fact_set_below(fact_set*, fact_set*);

void split_fact_set(fact_set*);

void delete_fact_set(fact_set*);

bool insert_state_fact_set(fact_set**, const clp_atom*, unsigned int step, constants*);
fact_set* insert_in_fact_set(fact_set*, const clp_atom*, unsigned int, constants*);

void print_state_fact_set(fact_set**, const constants*, FILE*, unsigned int n_predicates);
void print_fact_set(fact_set*, const constants*, FILE*);
bool is_in_fact_set(const fact_set*, const clp_atom*, constants*);
bool atom_true_in_fact_set(const fact_set*, const clp_atom*, substitution* sub, constants*);
unsigned int get_fact_set_timestamp(const fact_set*);
#endif
