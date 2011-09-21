/* substitution.c

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

#ifndef __INCLUDE_SUBSTITUTION_H
#define __INCLUDE_SUBSTITUTION_H

#include "theory.h"

/**
   An substitution of variables
   to term values

   The variables are those found in the freevars 
   set of the containing store
   
   Only "values" is used

   The pointer allvars points to the freevar of _all_ variables in the theory. 

   The timestamps are used to know when substitutions were inserted into the rete network.
   This is necessary for the emulation of the prolog search strategy in CL.pl
   The timestamps are a list of the timestamps when each conjunct in the precedent was 
   matched by the factset
**/
typedef struct substitution_t {
  unsigned int n_subs;
  signed int * timestamps;
  unsigned int n_timestamps;
  size_t size_timestamps;
  const freevars* allvars;
  const term* values[];
} substitution;


/**
   A linked list of substitutions to be used in 
   the alpha and beta stores. 
   The last elements are common between more threads, while
   the first ones are more specific for single threads
   
   prev is set when creating a sub_list_iter 
   prev is not threadsafe!
   
   The last element has next = NULL
**/
typedef struct substitution_list_t {
  struct substitution_list_t * next;
  struct substitution_list_t * prev;
  substitution * sub;
} substitution_list;

/**
   An iterator for substitution lists
**/
typedef substitution_list* sub_list_iter;

substitution* create_substitution(const theory*, signed int);
substitution* copy_substitution(const substitution*);
substitution* create_empty_fact_substitution(const theory*, const axiom*);



sub_list_iter* get_sub_list_iter(substitution_list*);
bool has_next_sub_list(const sub_list_iter*);
substitution* get_next_sub_list(sub_list_iter*);

const term* find_substitution(const substitution*, const variable*);
bool add_substitution(substitution*, variable*, const term*);
void insert_substitution_value(substitution*, variable*, const term*);
bool unify_substitution_terms(const term*, const term*, substitution*);
bool unify_substitution_term_lists(const term_list*, const term_list*, substitution*);
bool equal_substitutions(const substitution*, const substitution*, const freevars*);
bool subs_equal_intersection(const substitution*, const substitution*);
substitution* union_substitutions(const substitution*, const substitution*);

void delete_substitution(substitution*);
void delete_substitution_list(substitution_list*);
void delete_substitution_list_below(substitution_list*, substitution_list*);

void print_substitution(const substitution*, FILE*);
void print_coq_substitution(const substitution*, const freevars*, FILE*);
void print_substitution_list(const substitution_list*, FILE*);


// In instantiate.c
bool find_instantiate_sub(const atom* at, const atom* fact, substitution* sub);


bool test_substitution(const substitution*);
bool test_is_instantiation(const freevars*, const substitution*);
bool test_is_conj_instantiation(const conjunction*, const substitution*);

#endif
