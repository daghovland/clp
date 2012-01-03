/* rule_instance_state_stack.h

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
#ifndef __INCLUDED_RULE_INSTANCE_STATE_STACK_H
#define __INCLUDED_RULE_INSTANCE_STATE_STACK_H

#include "substitution.h"
#include "rule_queue.h"


typedef struct rule_instance_state_t {
  rule_instance* ri;
  rete_net_state* s;
  timestamp step_no;
} rule_instance_state;



/**
   Used in prover.c by the coq output to keep track 
   of the existential and disjunctive rules that have been eliminated, 
   but not yet proved
   
   Is a fifo / stack in array implementation
   
**/
typedef struct rule_instance_state_stack_t {
  size_t size_stack;
  rule_instance ** ris;
  rete_net_state ** states;
  timestamp* step_nos;
  unsigned int n_stack;
} rule_instance_state_stack;


rule_instance_state* create_rule_instance_state(rule_instance*, rete_net_state*, timestamp);
rule_instance_state_stack* initialize_ri_state_stack(void);
void push_ri_state_stack(rule_instance_state_stack*, rule_instance*, rete_net_state*, timestamp);
void add_ri_state_stack(rule_instance_state_stack* dest, rule_instance_state_stack* src);
void pop_ri_state_stack(rule_instance_state_stack*, rule_instance**, rete_net_state**, timestamp*);
bool is_empty_ri_state_stack(rule_instance_state_stack*);
void delete_ri_state_stack(rule_instance_state_stack*);
#endif
