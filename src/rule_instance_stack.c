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
#include "rule_queue.h"
#include "rule_instance_stack.h"

rule_instance_stack* initialize_ri_stack(void){
  rule_instance_stack* ris = malloc_tester(sizeof(rule_instance_stack));
  ris->size_stack = 5;
  ris->stack = calloc(ris->size_stack, sizeof(rule_instance*));
  ris->n_stack = 0;
  return ris;
}

void push_ri_stack(rule_instance_stack* ris, rule_instance* ri){
  while(ris->n_stack >= ris->size_stack - 1){
    ris->size_stack *= 2;
    ris->stack = realloc_tester(ris->stack, sizeof(rule_instance*) * ris->size_stack);
  }
  ris->stack[ris->n_stack] = ri;
  ris->n_stack++;
}

bool is_empty_ri_stack(rule_instance_stack * ris){
  return (ris->n_stack == 0);
}

rule_instance* pop_ri_stack(rule_instance_stack* ris){
  assert(ris->n_stack > 0);
  return ris->stack[--(ris->n_stack)];
}


void delete_ri_stack(rule_instance_stack* ris){
  free(ris->stack);
  free(ris);
}
