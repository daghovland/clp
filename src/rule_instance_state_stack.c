/* rule_instance_state_stack.c

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

#include "common.h"
#include "rete.h"
#include "rule_queue.h"
#include "rule_instance_state_stack.h"

rule_instance_state_stack* initialize_ri_state_stack(void){
  rule_instance_state_stack* ris = malloc_tester(sizeof(rule_instance_state_stack));
  ris->size_stack = 5;
  ris->ris = calloc(ris->size_stack, sizeof(rule_instance*));
  ris->states = calloc(ris->size_stack, sizeof(rete_net_state*));
  ris->step_nos = calloc(ris->size_stack, sizeof(timestamp));
  ris->n_stack = 0;
  return ris;
}

void increase_stack_size(rule_instance_state_stack* ris, unsigned int min_size){
  while(min_size >= ris->size_stack){
    ris->size_stack *= 2;
    ris->states = realloc_tester(ris->states, sizeof(rete_net_state*) * ris->size_stack);
    ris->ris = realloc_tester(ris->ris, sizeof(rule_instance*) * ris->size_stack);
    ris->step_nos = realloc_tester(ris->step_nos, sizeof(timestamp) * ris->size_stack);
  }
}

void add_ri_state_stack(rule_instance_state_stack* dest, rule_instance_state_stack* src){
  increase_stack_size(dest, dest->n_stack + src->n_stack + 1);
  memcpy(dest->states + dest->n_stack, src->states, src->n_stack * sizeof(rete_net_state*));
  memcpy(dest->ris + dest->n_stack, src->ris, src->n_stack * sizeof(rule_instance*));
  memcpy(dest->step_nos + dest->n_stack, src->step_nos, src->n_stack * sizeof(timestamp));
  dest->n_stack += src->n_stack;
}
    

rule_instance_state* create_rule_instance_state(rule_instance* ri, rete_net_state* s, timestamp step_no){
  rule_instance_state* ris = malloc_tester(sizeof(rule_instance_state));
  ris->ri = ri;
  ris->s = s;
  ris->step_no = step_no;
  return ris;
}

void push_ri_state_stack(rule_instance_state_stack* ris, rule_instance* ri, rete_net_state* s, timestamp step_no){
  increase_stack_size(ris, ris->n_stack + 1);
  ris->ris[ris->n_stack] = ri;
  ris->states[ris->n_stack] = s;
  ris->step_nos[ris->n_stack] = step_no;
  ++(ris->n_stack);
}

bool is_empty_ri_state_stack(rule_instance_state_stack * ris){
  return (ris->n_stack == 0);
}

void pop_ri_state_stack(rule_instance_state_stack* ris, rule_instance** ri, rete_net_state** s, timestamp * step_no){
  assert(ris->n_stack > 0);
  --(ris->n_stack);
  *ri = ris->ris[ris->n_stack];
  *s = ris->states[ris->n_stack];
  *step_no = ris->step_nos[ris->n_stack];
}


void delete_ri_state_stack(rule_instance_state_stack* ris){
  free(ris->step_nos);
  free(ris->states);
  free(ris->ris);
  free(ris);
}
