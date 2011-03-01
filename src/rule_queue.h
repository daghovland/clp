/* rete.h

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
#ifndef __INCLUDED_RULE_QUEUE_H
#define __INCLUDED_RULE_QUEUE_H

#include "substitution.h"
/**
   An instance of a rule is an axiom together
   with a term for each free variable

   The timestamp is the value of global_step_no at the time of creation
   This is used by next_instance in prover.c to choose what rule instance to apply
**/
typedef struct rule_instance_t {
  unsigned int timestamp;
  const axiom * rule;
  substitution * substitution;
} rule_instance;

/**
   The queue of rule instances to be treated
   Called "conflict set" by Forgy

   Is a linked list, because same parts are shared over threads

   The "parent" element is the older element, possible shared by more threads
   The top elements have parent NULL
**/
typedef struct rule_queue_t {
  size_t n_queue;
  size_t size_queue;
  size_t first;
  size_t end;
  unsigned int previous_appl;
  unsigned int n_appl;
  rule_instance* queue[];
} rule_queue;




rule_queue* initialize_queue(void);
rule_queue* copy_rule_queue(const rule_queue*);
void delete_queue(rule_queue*);
void delete_rule_instance(rule_instance*);
void delete_rule_queue(rule_queue*);

void print_rule_queue(const rule_queue*, FILE*);
void print_dot_rule_queue(const rule_queue*, FILE*);
void print_rule_instance(const rule_instance*, FILE*);
void print_dot_rule_instance(const rule_instance*, FILE*);

#endif
