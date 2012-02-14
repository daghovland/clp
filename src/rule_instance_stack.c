/* rule_instance_stack.c

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
#include "rule_instance_stack.h"

rule_instance_stack* initialize_ri_stack(void){
  rule_instance_stack* ris = malloc_tester(sizeof(rule_instance_stack));
  ris->size_stack = 5;
  ris->stack = calloc(ris->size_stack, sizeof(timestamp));
  ris->step_nos = calloc(ris->size_stack, sizeof(timestamp));
  ris->pusher_step_nos = calloc(ris->size_stack, sizeof(timestamp));
  ris->n_stack = 0;
  return ris;
}


void increase_ri_stack_size(rule_instance_stack* ris, unsigned int min_size){
  while(min_size >= ris->size_stack){
    ris->size_stack *= 2;
    ris->stack = realloc_tester(ris->stack, sizeof(timestamp) * ris->size_stack);
    ris->step_nos = realloc_tester(ris->step_nos, sizeof(timestamp) * ris->size_stack);
    ris->pusher_step_nos = realloc_tester(ris->pusher_step_nos, sizeof(timestamp) * ris->size_stack);
  }
}

void add_ri_stack(rule_instance_stack* dest, rule_instance_stack* src){
  increase_ri_stack_size(dest, dest->n_stack + src->n_stack + 1);
  memcpy(dest->stack + dest->n_stack, src->stack, src->n_stack * sizeof(timestamp));
  memcpy(dest->step_nos + dest->n_stack, src->step_nos, src->n_stack * sizeof(timestamp));
  memcpy(dest->pusher_step_nos + dest->n_stack, src->pusher_step_nos, src->n_stack * sizeof(timestamp));
  dest->n_stack += src->n_stack;
}
    
/**
   This is only called to from rete_state_single to push stuff on the stack for history
**/
void push_ri_stack(rule_instance_stack* ris, timestamp ri, timestamp step_no, timestamp pusher){
  while(ris->n_stack >= ris->size_stack - 1){
    ris->size_stack *= 2;
    ris->stack = realloc_tester(ris->stack, sizeof(timestamp) * ris->size_stack);
    ris->step_nos = realloc_tester(ris->step_nos, sizeof(timestamp) * ris->size_stack);
    ris->pusher_step_nos = realloc_tester(ris->pusher_step_nos, sizeof(timestamp) * ris->size_stack);
  }
  ris->stack[ris->n_stack] = ri;
  ris->step_nos[ris->n_stack] = step_no;
  ris->pusher_step_nos[ris->n_stack] = pusher;
  ris->n_stack++;
}

bool is_empty_ri_stack(rule_instance_stack * ris){
  return (ris->n_stack == 0);
}

bool next_ri_is_after(rule_instance_stack* ris, unsigned int limit){
  return (ris->n_stack > 0 && ris->step_nos[ris->n_stack - 1].step > limit);
}

timestamp pop_ri_stack(rule_instance_stack* ris, timestamp * step_no, timestamp *pusher){
  assert(ris->n_stack > 0);
  -- ris->n_stack;
  *step_no = ris->step_nos[ris->n_stack];
  *pusher = ris->pusher_step_nos[ris->n_stack];
  return ris->stack[ris->n_stack];
}

/**
   Called from write_mt_coq_proof after proof is done

   used for writing usages in reverse order
**/
void init_rev_stack(rule_instance_stack* ri){
  ri->n_2_stack = 0;
}

timestamp pop_rev_ri_stack(rule_instance_stack* ris, timestamp * step_no, timestamp* pusher){
  timestamp ri;
  assert(ris->n_2_stack < ris->n_stack);
  *step_no = ris->step_nos[ris->n_2_stack];
  *pusher = ris->pusher_step_nos[ris->n_2_stack];
  ri = ris->stack[ris->n_2_stack];
  ++ ris->n_2_stack;
  return ri;
}

bool is_empty_rev_ri_stack(rule_instance_stack * ris){
  return (ris->n_2_stack >= ris->n_stack);
}

/**
   Printing and deleting. Not sure if this is used.
**/

void delete_ri_stack(rule_instance_stack* ris){
  free(ris->stack);
  free(ris);
}

void print_coq_ri_stack(FILE* f, rule_instance_stack* ris){
  int i;
  fprintf(f, "(* ");
  if(ris->n_stack == 0)
    fprintf(f, "empty stack");
  else {
    for(i = ris->n_stack-1; i >= 0; i--)
      fprintf(f, "%i ", ris->step_nos[i].step);
  }
  fprintf(f, "*)\n");
}
