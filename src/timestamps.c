/* timestamps.c

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
   Auxiliary Code for updating state of rete network
**/
#include "common.h"
#include "timestamps.h"

/**
   Initializes already allocated substitution
   Only used directly by the factset_lhs implementation 
   in axiom_false_in_fact_set in rete_state.c
**/
void init_empty_timestamps(timestamps* ts, substitution_size_info ssi){
  unsigned int i;    
  ts->n_timestamps = 0;
  for(i = 0; i < get_max_n_timestamps(ssi); i++)
    ts->timestamps[i] = 0;
}


timestamps_iter get_timestamps_iter(const timestamps* ts){
  timestamps_iter iter;
  iter.n = 0;
  iter.ts = ts;
  return iter;
}

bool has_next_timestamps_iter(const timestamps_iter* iter){
  return iter->n < iter->ts->n_timestamps;
}

signed int get_next_timestamps_iter(timestamps_iter* iter){
  iter->n++;
  return iter->ts->timestamps[iter->n - 1];
}

void destroy_timestamps_iter(timestamps_iter* iter){
  ;
}

unsigned int get_n_timestamps(const timestamps* ts){
  return ts->n_timestamps;
}

/**
   Adds a single timestamp.

   Called from reamining_axiom_false_in_factset in rete_state.c

   Necessary for the output of correct coq proofs
**/
void add_timestamp(timestamps* ts, unsigned int timestamp){  
  ts->timestamps[ts->n_timestamps] = timestamp;
  ts->n_timestamps ++;  
}

/**
   Adds the timestamps in orig to those in dest.
   Called from union_substitutions_struct_with_ts in substitution.c
**/
void add_timestamps(timestamps* dest, const timestamps* orig){
  unsigned int i;
  for(i = 0; i < orig->n_timestamps; i++)
    dest->timestamps[dest->n_timestamps++] = orig->timestamps[i];
}
  

/**
   Internal function for comparing timestamps on a substition

   They correspond to the times at which the matching for each conjunct
   was introduced to the factset

   Returns positive if first is larger(newer) than last, negative if first is smaller(newer) than last,
   and 0 if they are equal
**/
int compare_timestamps(const timestamps* first, const timestamps* last){
  assert(first->n_timestamps == last->n_timestamps);
  unsigned int i;
  for(i = 0; i < first->n_timestamps; i++){
    if(first->timestamps[i] != last->timestamps[i])
      return first->timestamps[i] - last->timestamps[i];
  }
  return 0;
}
