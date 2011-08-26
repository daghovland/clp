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

#include "substitution.h"
#include "rule_queue.h"

/**
   Used in prover.c by the coq output to keep track 
   of the existential rules that have been eliminated, 
   but not yet proved
   
   Is a fifo / stack in array implementation
   
**/
typedef struct rule_instance_stack_t {
  size_t size_stack;
  rule_instance ** stack;
  unsigned int n_stack;
} rule_instance_stack;



rule_instance_stack* initialize_ri_stack(void);
void push_ri_stack(rule_instance_stack*, rule_instance*);
rule_instance* pop_ri_stack(rule_instance_stack*);
bool is_empty_ri_stack(rule_instance_stack*);
void delete_ri_stack(rule_instance_stack*);
#endif
