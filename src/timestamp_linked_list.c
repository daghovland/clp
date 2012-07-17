/* timestamp_linked_list.c

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
   Arrays of timestamps are used in substitutions, to keep
   track of the steps at which the left-hand side were first inferred.

   Included from timestamp_list _if_ USE_TIMESTAMP_ARRAY is defined in common.h
   Otherwise, timestamp_linked_list is used

   If there are n timestamps, there are n elements in the linked list. The oldest/first inserted has next pointer NULL
   the other n-1 have valid next pointer.
   
   New timestamps are added at the end "list"
   But union of timestamps is done by concatenation, so the order of the timestamps is not necessarily timestamp-ordered
**/
#include "common.h"
#ifdef USE_TIMESTAMP_ARRAY
#abort
#endif
#include "timestamps.h"
#include "term.h"
#include "substitution.h"
#include "rule_instance.h"
#include "error_handling.h"
#ifdef HAVE_PTHREAD
#include <pthread.h>
#endif


#ifndef NDEBUG
bool test_timestamps(const timestamps* ts){
#if false
  timestamp_linked_list* orig_el = ts->list;
  unsigned int ts_len = get_n_timestamps(ts);
  unsigned int count;
  if(orig_el != NULL){
    assert(ts->oldest != NULL);
    count = 1;
    if(orig_el->older != NULL){
      count++;
      while(orig_el->older != ts->oldest){
	assert(orig_el != NULL && orig_el -> older != NULL);
	orig_el = orig_el->older;
	count ++;
      }
      assert(ts->oldest->newer == orig_el);
    }
    assert(count == ts_len);
  }
  orig_el = ts->oldest;
  if(orig_el != NULL){
    assert(ts->list != NULL);
    count = 1;
    if(orig_el->newer != NULL){
      count++;
      while(orig_el->newer != ts->list){
	assert(orig_el != NULL && orig_el->newer != NULL);
	count ++;
	orig_el = orig_el->newer;
      }
      assert(ts->list->older == orig_el);
    }
    assert(count == ts_len);
  }
#endif
  return true;
}
#endif

/**
   Initializes already allocated substitution
   Only used directly by the factset_lhs implementation 
   in axiom_false_in_fact_set in rete_state.c

   Assumes the memory for the timestamp is already allocated
**/

void init_empty_timestamp_linked_list(timestamps* ts, bool permanent){
  ts->list = NULL;
  //  ts->oldest = NULL;
  ts->permanent = permanent;
}
void init_empty_timestamps(timestamps* ts, substitution_size_info ssi){
  init_empty_timestamp_linked_list(ts, false);
}


timestamp get_first_timestamp(timestamps* ts){
  assert(ts != NULL && ts->list != NULL);
  timestamp_linked_list* el = ts->list;
  while(el->older != NULL)
    el = el->older;
  return el->ts;
}

timestamps_iter get_timestamps_iter(const timestamps* ts){
  timestamps_iter iter;
  iter.ts_list = ts->list;
  while(iter.ts_list->older != NULL)
    iter.ts_list = iter.ts_list->older;
  return iter;
}

bool has_next_timestamps_iter(const timestamps_iter* iter){
  return iter->ts_list != NULL;
}

bool has_next_non_eq_timestamps_iter(const timestamps_iter* iter){
  return iter->ts_list != NULL && !is_equality_timestamp(iter->ts_list->ts);
}

timestamp get_next_timestamps_iter(timestamps_iter* iter){
  assert(iter != NULL);
  timestamp ts = iter->ts_list->ts;
  iter->ts_list = iter->ts_list->newer;
  return ts;
}

void destroy_timestamps_iter(timestamps_iter* iter){
  ;
}
 
unsigned int get_n_timestamps(const timestamps* ts){
  unsigned int n = 0;
  timestamp_linked_list * el = ts->list;
  while(el != NULL){
    el = el->older;
    n++;
  }
  return n;
}


/**
   returns true if there is a timestamp with the given step in lst
**/
bool timestamp_is_in_list(timestamp_linked_list* list, timestamp t){
  if(list == NULL)
    return false;
  if(compare_timestamp(list->ts, t) == 0)
    return true;
  return(timestamp_is_in_list(list->older, t));
}

/**
   Adds a single timestamp.

   Called from reamining_axiom_false_in_factset in rete_state.c

   Necessary for the output of correct coq proofs
**/
void add_timestamp(timestamps* ts, timestamp t, timestamp_store* store){  
  timestamp_linked_list* new_ts = get_timestamp_memory(store, ts->permanent);
#ifndef NDEBUG
  unsigned int new_ts_len;
  unsigned int orig_ts_len = get_n_timestamps(ts);
#endif
  assert(new_ts != NULL);
  new_ts->ts = t;
  new_ts->older = ts->list;
  new_ts->newer = NULL;
  if(ts->list != NULL)
    ts->list->newer = new_ts;
  ts->list = new_ts;
#if false
  if(ts->oldest == NULL){
    assert(ts->list->older == NULL);
    ts->oldest = new_ts;
  }
#endif
#ifndef NDEBUG
  new_ts_len = get_n_timestamps(ts);
  assert(orig_ts_len + 1 == new_ts_len);
  assert(test_timestamps(ts));
#endif
}

/**
   Adds the timestamps in orig to those in dest.
   Called from union_substitutions_struct_with_ts in substitution.c
**/
void add_timestamps(timestamps* dest, const timestamps* orig, timestamp_store* store){
  timestamp_linked_list* orig_el = orig->list;
  if(orig_el != NULL){
    while(orig_el->older != NULL)
      orig_el = orig_el->older;
  }
  assert(test_timestamps(dest));
  assert(test_timestamps(orig));
  while(orig_el != NULL){
    add_timestamp(dest, orig_el->ts, store);
    orig_el = orig_el->newer;
  }
  assert(test_timestamps(dest));
}

/**
   Called from copy_substitution_struct in substitution.c
**/
void copy_timestamps(timestamps* dest, const timestamps* orig, timestamp_store* store, bool permanent){
  init_empty_timestamp_linked_list(dest, permanent);
  add_timestamps(dest, orig, store);
}
/**
   Internal function for comparing timestamps on a substition

   They correspond to the times at which the matching for each conjunct
   was introduced to the factset

   Returns positive if first is larger(newer) than last, negative if oldest is smaller(older) than last,
   and 0 if they are equal
**/
int compare_timestamps(const timestamps* first, const timestamps* last){
  timestamp_linked_list *first_el = first->list;
  timestamp_linked_list *last_el = last->list;
  assert(first_el != NULL || last_el == NULL);
  while(first_el != NULL){
    assert(last_el != NULL);
    if(first_el->ts.step != last_el->ts.step)
      return first_el->ts.step - last_el->ts.step;
    first_el = first_el->older;
    last_el = last_el->older;
  }
  assert(last_el == NULL);
  return 0;
}

/**
   Discovers how much space is needed for the timestmaps, substitutions and rule instances

   This is declared in substitution_size_info.h
**/
substitution_size_info init_sub_size_info(unsigned int n_vars, unsigned int max_lhs_conjuncts){
  substitution_size_info ssi;
  unsigned int size_vars, size_timestamps;
  ssi.max_n_timestamps = max_lhs_conjuncts + 1;
  size_vars = n_vars * sizeof(clp_term*);
  size_timestamps = 0;
  ssi.size_substitution = sizeof(substitution) + size_vars;
  ssi.size_rule_instance = sizeof(rule_instance) + size_vars;
  ssi.sub_values_offset =  0;
  return ssi;
}

