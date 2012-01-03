/* rule_instance_stack.h

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
#ifndef __INCLUDED_RULE_INSTANCE_STACK_H
#define __INCLUDED_RULE_INSTANCE_STACK_H

#include "common.h"

/**
   Used in prover.c by the coq output to keep track 
   of the existential and disjunctive rules that have been eliminated, 
   but not yet proved

   The rule instances are referred to by their step in the history
   
   Is a fifo / stack in array implementation
   
**/
typedef struct rule_instance_stack_t {
  size_t size_stack;
  timestamp * stack;
  timestamp * step_nos;
  timestamp * pusher_step_nos;
  unsigned int n_stack;
  unsigned int n_2_stack;
} rule_instance_stack;



rule_instance_stack* initialize_ri_stack(void);
void init_rev_stack(rule_instance_stack*);
void push_ri_stack(rule_instance_stack*, timestamp, timestamp, timestamp);
timestamp pop_ri_stack(rule_instance_stack*, timestamp*, timestamp*);
timestamp pop_rev_ri_stack(rule_instance_stack*, timestamp*, timestamp*);
void add_ri_stack(rule_instance_stack* dest, rule_instance_stack* src);
bool is_empty_ri_stack(rule_instance_stack*);
bool is_empty_rev_ri_stack(rule_instance_stack*);
void delete_ri_stack(rule_instance_stack*);
void print_coq_ri_stack(FILE*, rule_instance_stack*);
#endif
