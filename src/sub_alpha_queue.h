/* substitution.h

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

#ifndef __INCLUDE_SUB_ALPHA_QUEUE_H
#define __INCLUDE_SUB_ALPHA_QUEUE_H

#include "substitution.h"
#include "rete_node.h"

/**
   
   Stores a linked list where each element has a substitution and an alpha node.

   Each rule node has such a queue, used for the "lazy" insertion of substitutions
   The last elements are common between more threads, while
   the first ones are more specific for single threads
   
   prev is set when creating a sub_list_iter 
   prev is not threadsafe!
   
   The last element has next = NULL
**/
typedef struct sub_alpha_queue_t {
  struct sub_alpha_queue_t * next;
#ifndef HAVE_PTHREAD
  struct sub_alpha_queue_t * prev;
#endif
  substitution * sub;
  rete_node * alpha_node;
} sub_alpha_queue;

/**
   An iterator for substitution lists
**/
typedef sub_alpha_queue* sub_alpha_queue_iter;

void delete_sub_alpha_queue_below(sub_alpha_queue* list, sub_alpha_queue* limit);
void delete_sub_alpha_queue(sub_alpha_queue* sub_list);


sub_alpha_queue_iter* get_sub_alpha_queue_iter(sub_alpha_queue*);
bool has_next_sub_alpha_queue(const sub_alpha_queue_iter*);
void get_next_sub_alpha_queue(sub_alpha_queue_iter*, substitution**, rete_node**);



#endif
