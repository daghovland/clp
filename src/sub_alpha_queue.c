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
#include "rete_state.h"

sub_alpha_queue init_sub_alpha_queue(void){
  sub_alpha_queue queue;
  queue.root = NULL;
  queue.end = NULL;
  return queue;
}

/**
   Insert pair of substitution and alpha node into list.

   This function creates a new "sub_alpha_queue" element on the heap
   This element must be deleted when backtracking by calling delete_sub_alpha_queue_below

   The substitution is put on the list as it is (not copied), and must not be changed or deleted by the calling function.
   The substitution is deleted when the list is deleted on backtracking

   Before the call, the sub_list_ptr points to a pointer which has the previous sub list, and after the
   call, this will point to the new sub_alpha_queue.
**/
bool insert_in_sub_alpha_queue(sub_alpha_queue * queue,
			       const clp_atom * fact, 
			       substitution* a, 
			       const rete_node* alpha_node){
  sub_alpha_queue_elem* sub_list = queue->end;

  assert(test_substitution(a));
  assert(alpha_node->type == alpha);
  
  
  queue->end = malloc_tester(sizeof(sub_alpha_queue));
  queue->end->sub = a;
  queue->end->fact = fact;
  queue->end->alpha_node = alpha_node;
  queue->end->next = sub_list;
  sub_list->prev = queue->end;
  queue->end->is_splitting_point = false;
  if(queue->root == NULL)
    queue->root = queue->end;
  assert(test_substitution(a));
  
  return true;
}

/**
   Called from delete_rete_state in rete.c

   Deletes all sub_alpha_queues on the path from list which are newer than limit 
   when following the "next" pointer. 
   Uses the timestamp in the substitutions.
**/
void delete_sub_alpha_queue_below(sub_alpha_queue* queue, sub_alpha_queue* limit_queue){
  //  sub_alpha_queue * above_limit;
  
  /*
    if(list == limit || list == NULL)
    return;
    
    for(above_limit = limit; above_limit != NULL; above_limit = above_limit->next){
    if(above_limit == list)
    return;
    }
  */
  sub_alpha_queue_elem * limit = limit_queue->end;
  while(queue->end != NULL && queue->end != limit && ( limit == NULL || compare_sub_timestamps(queue->end->sub, limit->sub) > 0 )){
    assert(limit == NULL || compare_sub_timestamps(queue->end->sub, limit->sub) > 0 );
    assert(queue->end != NULL);
    sub_alpha_queue_elem* next = queue->end->next;
    free(queue->end);
    queue->end = next;
  }
}

bool is_empty_sub_alpha_queue(sub_alpha_queue* root){
  return root == NULL;
}

/**
   Assume the "root" has a non-null prev pointer. This element replaces the root
   The adress of the current root substitution and fact are put into the given addresses
 **/
void pop_sub_alpha_queue_mt(sub_alpha_queue* queue, substitution** sub, const clp_atom** fact, const rete_node** alpha_node){
  assert(queue->root->next == NULL);
  *sub = queue->root->sub;
  *fact = queue->root->fact;
  *alpha_node = queue->root->alpha_node;
  queue->root = queue->root->prev;
  if(queue->root == NULL)
    queue->end = NULL;
  else 
    queue->root->next = NULL;
}

/**
   Helper function for axiom_has_new_instance

   If the the rete node is used for the rhs, this only checks that
   there is something on the queue.

   Otherwise, if the factset is used, it searches for any instances
   with a rhs not fulfilled, and discards the others
**/
bool axiom_queue_has_interesting_instance(size_t axiom_no, rete_net_state* state){
  
  while(!is_empty_axiom_rule_queue(state, axiom_no)){
    substitution* sub;
    rule_instance* next;
    if(state->net->use_beta_not)
      return true;
    next = peek_axiom_rule_queue(state, axiom_no);
    sub = copy_substitution(& next->sub, & state->local_subst_mem, state->net->th->sub_size_info);
    if(!disjunction_true_in_fact_set(state, next->rule->rhs, sub)){
      return true;
    }
    //delete_rule_instance(pop_axiom_rule_queue(state, axiom_no));
    pop_axiom_rule_queue(state, axiom_no);
  }
  return false;
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

bool axiom_has_new_instance(rule_queue_state rqs, unsigned int axiom_no){
  rete_net_state * state = rqs.state;
  if(state->net->factset_lhs)
    return !is_empty_axiom_rule_queue(state, axiom_no);
  else {
    sub_alpha_queue * sub_queue = & state->sub_alpha_queues[axiom_no];
    if(axiom_queue_has_interesting_instance(axiom_no, state))
      return true;
    
    while(sub_queue->end != sub_queue->root && sub_queue->end != NULL){
      sub_alpha_queue_elem* new_root;
      
      // TODO: This might be a performance hit if the queues become very long. 
      for( new_root = sub_queue->end; new_root->next != sub_queue->root; new_root = new_root->next)
	;
      
      insert_rete_alpha_fact_children(state, 
				      new_root->alpha_node, 
				      new_root->fact, 
				      copy_substitution(new_root->sub, & state->local_subst_mem, state->net->th->sub_size_info), 
				      true);
      
      state->sub_alpha_queues[axiom_no].root = new_root;
      
      sub_queue->root = new_root; 
      
      if(axiom_queue_has_interesting_instance(axiom_no, state))
	return true;
    }
    return false;
  }
}

/**
   Necessary for the efficiency of the lazy version of rete
**/
bool axiom_may_have_new_instance(rule_queue_state rqs, unsigned int axiom_no){
  rete_net_state* state = rqs.state;
  if(state->net->factset_lhs)
     return !is_empty_axiom_rule_queue(state, axiom_no);
  return( (!is_empty_axiom_rule_queue(state, axiom_no)) || !is_empty_sub_alpha_queue(&state->sub_alpha_queues[axiom_no]));
}

/**
   In the lazy version of rete, we calculuate the maximum age of any instance of a rule by taking either the oldest rule instance already in the queue, 
   or if the queue is empty, the oldest substitution waiting to be inserted. 

   This avoids a bottleneck that occurs when we force at least one instance on the queue when possible.
**/
unsigned int rule_queue_possible_age(rule_queue_state rqs, unsigned int axiom_no){
  timestamp ts;
  rete_net_state* state = rqs.state;
  if(!is_empty_axiom_rule_queue(state, axiom_no)){
    rule_instance * ri = peek_axiom_rule_queue(state, axiom_no);
    return ri->timestamp;
  } else {
    sub_alpha_queue_elem *new_root, * root;
    root = state->sub_alpha_queues[axiom_no].root;
    assert(NULL != root);
    // TODO: This might be a performance hit if the queues become very long. 
    for( new_root = state->sub_alpha_queues[axiom_no].root; new_root->next != root; new_root = new_root->next)
      ;
    ts = get_oldest_timestamp(& new_root->sub->sub_ts);
    return ts.step;
  }
}
