/* substitution.h

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

#ifndef __INCLUDED_SUBSTITUTION_H
#define __INCLUDED_SUBSTITUTION_H

#include "substitution_struct.h"
#include "substitution_store_mt.h"
#include "theory.h"
#include "substitution_size_info.h"

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

substitution* create_empty_substitution(const theory*, substitution_store_mt*);
void init_empty_substitution(substitution*, const theory*);
void init_substitution(substitution*, const theory*, signed int, timestamp_store*);
substitution* create_substitution(const theory*, signed int, substitution_store_mt*, timestamp_store*, const constants* cs);
substitution* copy_substitution(const substitution*, substitution_store_mt*, substitution_size_info, timestamp_store*, const constants*);
void copy_substitution_struct(substitution*, const substitution*, substitution_size_info, timestamp_store*, bool permanent, const constants*);
substitution* create_empty_fact_substitution(const theory*, const clp_axiom*, substitution_store_mt*, timestamp_store*, const constants* cs);

unsigned int get_sub_n_timestamps(const substitution*);
timestamps_iter get_sub_timestamps_iter(const substitution*);
int compare_sub_timestamps(const substitution*, const substitution*);

const clp_term* const * get_sub_values_ptr(const substitution*);

sub_list_iter* get_sub_list_iter(substitution_list*);
bool has_next_sub_list(const sub_list_iter*);
substitution* get_next_sub_list(sub_list_iter*);
void free_sub_list_iter(sub_list_iter*);

void free_substitution(substitution*);
//void copy_timestamps(substitution* into, const substitution* orig, substitution_size_info);
const clp_term* find_substitution(const substitution*, const clp_variable*, const constants*);
bool add_substitution(substitution*, clp_variable*, const clp_term*, constants*, timestamp_store*, bool update_ts);
void insert_substitution_value(substitution*, clp_variable*, const clp_term*, const constants*);
bool unify_substitution_terms(const clp_term*, const clp_term*, substitution*, constants*, timestamp_store*);
bool unify_substitution_term_lists(const term_list*, const term_list*, substitution*, constants*, timestamp_store*);
bool equal_substitutions(const substitution*, const substitution*, const freevars*, constants*);
bool literally_equal_substitutions(const substitution*, const substitution*, const freevars*, constants*);
bool subs_equal_intersection(const substitution*, const substitution*, constants*);
bool union_substitutions_struct_with_ts(substitution* dest, const substitution* sub1, const substitution* sub2, substitution_size_info, constants* cs, timestamp_store*);
bool union_substitutions_struct_one_ts(substitution* dest, const substitution* sub1, const substitution* sub2, substitution_size_info, constants*, timestamp_store*);
substitution* union_substitutions_one_ts(const substitution*, const substitution*, substitution_store_mt* store, substitution_size_info ssi, constants* cs, timestamp_store*);
substitution* union_substitutions_with_ts(const substitution*, const substitution*, substitution_store_mt* store, substitution_size_info ssi, constants*, timestamp_store*);
void add_sub_timestamp(substitution*, unsigned int, substitution_size_info, timestamp_store*);

void delete_substitution_list(substitution_list*);
void delete_substitution_list_below(substitution_list*, substitution_list*);

void print_substitution(const substitution*, const constants*, FILE*);
void print_coq_substitution(const substitution*, const constants*, const freevars*, FILE*);
void print_substitution_list(const substitution_list*, const constants*, FILE*);



bool test_substitution(const substitution*, const constants*);
bool test_is_instantiation(const freevars*, const substitution*, const constants*);

#endif
