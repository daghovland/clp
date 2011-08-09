/* sub_alpha_queue.c

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
#include "term.h"
#include "fresh_constants.h"
#include "rete.h"
#include "theory.h"
#include "substitution.h"
#include "sub_alpha_queue.h"

/**
   TODO: Many of these function are identical 
   (except for the substitution_list -> sub_alpha queue) 
   to the corresponding functions in subsitution.c
**/
void delete_sub_alpha_queue(sub_alpha_queue* sub_list){
  while(sub_list != NULL){
    sub_alpha_queue* next = sub_list->next;
    delete_substitution(sub_list->sub);
    free(sub_list);
    sub_list = next;
  }
}

void delete_sub_alpha_queue_below(sub_alpha_queue* list, sub_alpha_queue* limit){
  assert(limit != NULL);
  while(list != limit){
    assert(list != NULL);
    sub_alpha_queue* next = list->next;
    delete_substitution(list->sub);
    free(list);
    list = next;
  }
}

/**
   Iterator functionality

   An iterator is just a pointer to a sub_alpha_queue. 
   A pointer to an iterator therefore a pointer to a pointer to a sub_alpha_queue

   The substitutions must be returned in order: The oldest first. Therefore the
   prev pointer is used. This is not thread safe!!!
**/
sub_alpha_queue_iter* get_sub_alpha_queue_iter(sub_alpha_queue* sub){
  sub_alpha_queue_iter* i = malloc_tester(sizeof(sub_alpha_queue_iter));
  sub_alpha_queue* prev = NULL;

  while(sub != NULL){
    sub->prev = prev;
    prev = sub;    
    sub = sub->next;
  }
  *i = prev;
    
  return i;
}


bool has_next_sub_alpha_queue(const sub_alpha_queue_iter*i){
  return *i != NULL;
}

/**
   As mentioned above, this is not thread safe, since
   subsitution lists are shared between threads, and
   prev may differ. prev is set to NULL as soon as possible 
   to ensure early segfaults on erroneous usage.
**/
void get_next_sub_alpha_queue(sub_alpha_queue_iter* i,
			      substitution ** sub,
			      rete_node ** alpha_node){
  sub_alpha_queue * sl = *i;
  assert(*i != NULL);
  *sub = sl->sub;
  *alpha_node = sl->alpha_node;
  (*i) = sl->prev;
  sl->prev = NULL;
}

/**
   Insert pair of substitution and alpha node into list.

   This function creates a new "sub_alpha_queue" element on the heap
   This element must be deleted when backtracking by calling delete_sub_alpha_queue_below

   The substitution is put on the list as it is (not copied), and must not be changed or deleted by the calling function.

**/
bool insert_in_sub_alpha_queue(rete_net_state* state, 
			       size_t sub_no, 
			       substitution* a, 
			       rete_node* alpha_node){
  sub_alpha_queue* sub_list = state->sub_alpha_queues[sub_no];
  
  assert(test_substitution(a));
  assert(state->net->n_subs >= sub_no);
  assert(alpha_node->type == alpha);
  
  state->sub_alpha_queues[sub_no] = malloc_tester(sizeof(sub_alpha_queue));
  state->sub_alpha_queues[sub_no]->sub = a;
  state->sub_alpha_queues[sub_no]->alpha_node = alpha_node;
  state->sub_alpha_queues[sub_no]->next = sub_list;

  assert(test_substitution(a));
  
  return true;
}
