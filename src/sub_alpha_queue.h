/* sub_alpha_queue.h

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

#ifndef __INCLUDED_SUB_ALPHA_QUEUE_H
#define __INCLUDED_SUB_ALPHA_QUEUE_H

#include "substitution.h"
#include "rete_node.h"

/**
   
   Stores a linked list where each element has a substitution and an alpha node.

   Each rule node has such a queue, used for the "lazy" insertion of substitutions
   The last elements are common between more threads, while
   the first ones are more specific for single threads
   
   
   
   The last element has next = NULL

   is_splitting_point indicates whether it is the last queue element before a split
   It is set to false when constructed in insert_in_sub_alpha_queue, and set to true in split_rete_state (rete_state.c)
**/
typedef struct sub_alpha_queue_elem_t {
  struct sub_alpha_queue_elem_t * next;
  struct sub_alpha_queue_elem_t * prev;
  substitution * sub;
  const rete_node * alpha_node;
  const clp_atom * fact;
  bool is_splitting_point;
} sub_alpha_queue_elem;

typedef struct sub_alpha_queue_t {
  sub_alpha_queue_elem* root;
  sub_alpha_queue_elem* end;
} sub_alpha_queue;

sub_alpha_queue init_sub_alpha_queue(void);
void delete_sub_alpha_queue_below(sub_alpha_queue* list, sub_alpha_queue* limit);
bool insert_in_sub_alpha_queue(sub_alpha_queue *,
			       const clp_atom * fact, 
			       substitution* a, 
			       const rete_node* alpha_node);
void pop_sub_alpha_queue_mt(sub_alpha_queue*, substitution**, const clp_atom**, const rete_node**);
bool is_empty_sub_alpha_queue(sub_alpha_queue*);
#endif
