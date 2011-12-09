/* rete_insert.c

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
/**
   Code for updating state of rete network
**/
#include "common.h"
#include "term.h"
#include "fresh_constants.h"
#include "rete.h"
#include "substitution.h"
#include "sub_alpha_queue.h"

bool insert_rete_alpha_fact(rete_net_state*, const rete_node*,  const atom* , substitution*, bool);
/**
   Removing a substitution from a rete network
   Happens when inserting new facts
   
   Ultimately may lead to removing rule instances from the queues

   The substitution list sub is either put into a store and a rule queue or deleted
   The calling function must not touch sub after calling this function
 **/
void detract_rete_beta_sub(rete_net_state* state, 
			   const rete_node* node,
			   substitution* sub)
{
  sub_list_iter* iter;
  size_t sub_no = (node->type == beta_not) ? node->val.beta.b_store_no : node->val.rule.store_no;
  assert(node->type == beta_not || node->type == rule);

  iter = get_state_sub_list_iter(state, sub_no);

#ifdef __DEBUG_RETE_STATE
  fprintf(stdout, "\nRetracting: ");
  print_substitution(sub, state->constants, stdout);
#endif

  while(has_next_sub_list(iter)){
    substitution* sub2 = get_next_sub_list(iter);
    if(subs_equal_intersection(sub, sub2, state->constants)){
      if(node->type == beta_not)
	detract_rete_beta_sub(state, node->children[0], union_substitutions_one_ts(sub, sub2, & state->local_subst_mem, state->net->th->sub_size_info, state->constants));
      else 
	remove_rule_instance(state, sub2, node->val.rule.axm->axiom_no);
    }
  } // end   while(has_next_sub_list(iter)){
  free_state_sub_list_iter(state, sub_no, iter);
}

/**
   Inserting a substitution into a beta node in a rete network

   The substitution sub is either stored in a substitution list or deleted
   The calling function must not use sub after it has passed it to this function
**/
void insert_rete_beta_sub(rete_net_state* state, 
			  const rete_node* parent, 
			  const rete_node* node, 
			  substitution* sub)
{
  sub_list_iter* iter;

  assert(parent == node->left_parent || parent == node->val.beta.right_parent);
  assert(test_substitution(sub));
  
  if(node->type == rule){
    assert(node->n_children == 0);
    if(insert_substitution(state, 
			   node->val.rule.store_no, 
			   sub, node->free_vars,
			   state->constants
			   ))
      {
	assert(test_substitution(sub));
	add_rule_to_queue(node->val.rule.axm, sub, state);
      }
  } else {

    assert(node->type == beta_and || node->type==beta_not);
    assert(node->n_children == 1);	
    assert(node->left_parent != NULL && node->left_parent->type != alpha);
    
    
    switch(node->type){
    case beta_and:
      if(insert_substitution(state, 
			     node->val.beta.b_store_no, 
			     sub, node->free_vars,
			     state->constants	
			     ))
	{
	  iter = get_state_sub_list_iter(state, node->val.beta.a_store_no);
	  while(has_next_sub_list(iter)){
	    substitution* join;
	    if(node->in_positive_lhs_part)
	      join = union_substitutions_with_ts(sub, get_next_sub_list(iter), & state->local_subst_mem, state->net->th->sub_size_info, state->constants);
	    else
	      join = union_substitutions_one_ts(sub, get_next_sub_list(iter), & state->local_subst_mem, state->net->th->sub_size_info, state->constants);
	    if(join != NULL) 
	      insert_rete_beta_sub(state, node, node->children[0], join);
	  }
	  free_state_sub_list_iter(state, node->val.beta.a_store_no, iter);
	}
      break;
    case beta_not:
      if(parent == node->left_parent){
	if(insert_substitution(state, 
			       node->val.beta.b_store_no, 
			       sub, node->free_vars,
			       state->constants
			       ))
	  {
	    iter = get_state_sub_list_iter(state, node->val.beta.a_store_no);
	    while(has_next_sub_list(iter)){
	      if(subs_equal_intersection(sub, get_next_sub_list(iter), state->constants)){
		free_state_sub_list_iter(state, node->val.beta.a_store_no, iter);
		return;
	      }
	    }
	    free_state_sub_list_iter(state, node->val.beta.a_store_no, iter);
	    insert_rete_beta_sub(state, node, node->children[0], copy_substitution(sub, & state->local_subst_mem, state->net->th->sub_size_info));
	  }
      } else // right parent 
	if(insert_substitution(state, 
			       node->val.beta.a_store_no, 
			       sub, node->free_vars,
			       state->constants
			       ))
	  detract_rete_beta_sub(state, node->children[0], copy_substitution(sub, & state->local_subst_mem, state->net->th->sub_size_info));
      break;
    default:
      fprintf(stderr, "Encountered untreated node type: %i\n", node->type);
      assert(false);
    }
  } /* if(node->type == rule) ... else ... */
  return;
}

/**
   Inserting fact into all alpha children 
   Called from insert_rete_alpha_fact on selectors and alpha nodes
   
   propagate is true when called from axiom_has_new_instance in lazy_rule_queue.c, otherwise false.
   true propagate overrides the value of node->propagate

   The substitution is deleted, the calling function must not touch it after passing to this function.
**/
void insert_rete_alpha_fact_children(rete_net_state* state, const rete_node* node, const atom* fact, substitution* sub, bool propagate){
  unsigned int i;
  assert(node->type == selector || node->type == alpha);
  for(i = 0; i < node->n_children; i++)
    insert_rete_alpha_fact(state, node->children[i], fact, copy_substitution(sub, & state->local_subst_mem, state->net->th->sub_size_info), propagate);
}

/**
   The substitution sub is a pointer to heap memory which will
   be deallocated or stored in a substitution list. The calling
   function must not touch sub after passing it to this function, as it
   is stored or deleted as it is.

   propagate is true when call originates from axiom_has_new_instance in lazy_rule_queue.c, otherwise false.
   true propagate overrides the value of node->propagate
**/
bool insert_rete_alpha_fact(rete_net_state* state, 
			    const rete_node* node, 
			    const atom* fact, 
			    substitution* sub,
			    bool propagate)
{
  const term *arg;
  sub_list_iter* iter;

  assert(test_substitution(sub));

  switch(node->type){
  case selector:
    assert(node->val.selector == fact->pred);
    insert_rete_alpha_fact_children(state, node, fact, sub, false);
    break;
  case alpha:
    assert(node->val.alpha.argument_no < fact->args->n_args);
    arg = fact->args->args[node->val.alpha.argument_no];
    assert(test_term(arg));
    if( ! unify_substitution_terms(arg, node->val.alpha.value, sub, state->constants)){
      return false;
    }
#ifdef LAZY_RETE
    if(propagate || node->val.alpha.propagate){
#endif
      insert_rete_alpha_fact_children(state, node, fact, sub, false);
#ifdef LAZY_RETE
    } else {
      insert_in_sub_alpha_queue(& (state->sub_alpha_queues[node->rule_no]), fact, sub, node);
    }
#endif
    break;
  case beta_and:
    assert(node->n_children == 1);
    if(!insert_substitution(state, node->val.beta.a_store_no, sub, node->free_vars, state->constants)){  
      return false;
    }
    if(node->left_parent->type == beta_root){
      insert_rete_beta_sub(state, node, node->children[0], copy_substitution(sub, & state->local_subst_mem, state->net->th->sub_size_info));
    } else {
      iter = get_state_sub_list_iter(state, node->val.beta.b_store_no);
      while(has_next_sub_list(iter)){
	substitution* join;
	if(node->in_positive_lhs_part)
	  join = union_substitutions_with_ts(get_next_sub_list(iter), sub, & state->local_subst_mem, state->net->th->sub_size_info, state->constants);
	else
	  join = union_substitutions_one_ts(get_next_sub_list(iter), sub, & state->local_subst_mem, state->net->th->sub_size_info, state->constants);
	if(join != NULL) 
	  insert_rete_beta_sub(state, node, node->children[0], join);
      }
      free_state_sub_list_iter(state, node->val.beta.b_store_no, iter);
    }
    break;
  case beta_not: 
    if(insert_substitution(state, 
			   node->val.beta.a_store_no, 
			   sub, node->free_vars,
			   state->constants
			   ))
      detract_rete_beta_sub(state, node->children[0], sub);
    else {
      return false;
    }
    break;
  default:
    fprintf(stderr, "Encountered wrong node type %i\n", node->type);
    assert(false);
  }

  return true;
}    

/**
   Inserts fact into rete network.

   An empty substitution is created on the heap and passed to the network
   This must not be touched after the call to insert_rete_alpha_fact, since it
   deletes or stores it
**/
void insert_rete_net_fact(rete_net_state* state, 
			  const atom* fact){
  const rete_node* sel = get_const_selector(fact->pred->pred_no, state->net);
  assert(test_atom(fact));
  assert(sel != NULL && sel->val.selector == fact->pred);
  substitution* a = create_substitution(state->net->th, get_current_state_step_no(state), & state->local_subst_mem);
  insert_rete_alpha_fact(state, sel, fact, a, false);
}
