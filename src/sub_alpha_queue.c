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
   Insert pair of substitution and alpha node into list.

   This function creates a new "sub_alpha_queue" element on the heap
   This element must be deleted when backtracking by calling delete_sub_alpha_queue_below

   The substitution is put on the list as it is (not copied), and must not be changed or deleted by the calling function.
   The substitution is deleted when the list is deleted on backtracking

   Before the call, the sub_list_ptr points to a pointer which has the previous sub list, and after the
   call, this will point to the new sub_alpha_queue.
**/
bool insert_in_sub_alpha_queue(sub_alpha_queue ** sub_list_ptr,
			       const atom * fact, 
			       substitution* a, 
			       rete_node* alpha_node){
  sub_alpha_queue* sub_list = (sub_list_ptr == NULL) ? NULL : *sub_list_ptr; 
  
  assert(test_substitution(a));
  assert(alpha_node->type == alpha);

  
  *sub_list_ptr = malloc_tester(sizeof(sub_alpha_queue));
  (*sub_list_ptr)->sub = a;
  (*sub_list_ptr)->fact = fact;
  (*sub_list_ptr)->alpha_node = alpha_node;
  (*sub_list_ptr)->next = sub_list;
  (*sub_list_ptr)->is_splitting_point = false;
  (*sub_list_ptr)->prev = NULL;

  assert(test_substitution(a));
  
  return true;
}

/**
   Called from delete_rete_state in rete.c

   Deletes all sub_alpha_queues on the path from list which are newer than limit 
   when following the "next" pointer. 
   Uses the timestamp in the substitutions.
**/
void delete_sub_alpha_queue_below(sub_alpha_queue* list, sub_alpha_queue* limit){
  //  sub_alpha_queue * above_limit;
  
  /*
    if(list == limit || list == NULL)
    return;
    
    for(above_limit = limit; above_limit != NULL; above_limit = above_limit->next){
    if(above_limit == list)
    return;
    }
  */
  while(list != NULL && ( limit == NULL || list->sub->timestamps[0] > limit->sub->timestamps[0]) ){
    assert(limit == NULL || list->sub->timestamps[0] > limit->sub->timestamps[0]);
    assert(list != NULL);
    sub_alpha_queue* next = list->next;
    delete_substitution(list->sub);
    free(list);
    list = next;
  }
}


/*
  Implements functionality for the lazy version of RETE. 
  That is, querying whether
  there is a rule in the queue for an axiom, or if not, whether
  there is a substitution waiting to be inserted into some alpha node.

  Called from functions in strategy.c

  The value of is_splitting_point is used to decide whether the sub_alpha_queue and the
  contained substitution should be deleted.
*/

bool axiom_has_new_instance(size_t axiom_no, rete_net_state * state){
  sub_alpha_queue * sub_list = state->sub_alpha_queues[axiom_no];
  if(state->axiom_inst_queue[axiom_no]->n_queue > 0)
    return true;

  while(sub_list != NULL){
    insert_rete_alpha_fact_children(state, sub_list->alpha_node, sub_list->fact, copy_substitution(sub_list->sub), true);

    assert(sub_list->prev == NULL);

    state->sub_alpha_queues[axiom_no] = sub_list->next;
      
    if(! sub_list->is_splitting_point){
      delete_substitution(sub_list->sub);
      free(sub_list);
    } else {
      if(sub_list->next != NULL)
	sub_list->next->is_splitting_point = true;
    }

    sub_list = state->sub_alpha_queues[axiom_no];

    if(state->axiom_inst_queue[axiom_no]->n_queue > 0)
      return true;
  }
  return false;
}

