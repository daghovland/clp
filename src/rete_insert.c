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

bool insert_rete_alpha_fact(rete_net_state*, const rete_node*,  const atom* , substitution*);
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

  assert(node->type == beta_not || node->type == rule);

  iter = get_state_sub_list_iter(state, 
				 (node->type == beta_not) ? node->val.beta.b_store_no : node->val.rule.store_no
				 );

#ifdef __DEBUG_RETE_STATE
  fprintf(stdout, "\nRetracting: ");
  print_substitution(sub, stdout);
#endif

  while(has_next_sub_list(iter)){
    substitution* sub2 = get_next_sub_list(iter);
    if(subs_equal_intersection(sub, sub2))
      if(node->type == beta_not)
	detract_rete_beta_sub(state, node->children[0], union_substitutions(sub, sub2));
      else {
	assert(test_rule_queue_sums(state));
	remove_rule_instance(state, sub2, node->val.rule.axm->axiom_no);
	assert(test_rule_queue_sums(state));
      }
  } // end   while(has_next_sub_list(iter)){
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
			   sub, node->free_vars
			   ))
      {
	assert(test_substitution(sub));
	add_rule_to_queue(node->val.rule.axm, sub, state);
      } else
      delete_substitution(sub);
  } else {

    assert(node->type == beta_and || node->type==beta_not);
    assert(node->n_children == 1);	
    assert(node->left_parent != NULL && node->left_parent->type != alpha);
    
    
    switch(node->type){
    case beta_and:
      if(insert_substitution(state, 
			     node->val.beta.b_store_no, 
			     sub, node->free_vars
			     ))
	{
	  iter = get_state_sub_list_iter(state, node->val.beta.a_store_no);
	  while(has_next_sub_list(iter)){
	    substitution* join = union_substitutions(sub, get_next_sub_list(iter));
	    if(join != NULL) 
	      insert_rete_beta_sub(state, node, node->children[0], join);
	  }
	} else
	delete_substitution(sub);
      break;
    case beta_not:
      if(parent == node->left_parent){
	if(insert_substitution(state, 
			       node->val.beta.b_store_no, 
			       sub, node->free_vars
			       ))
	  {
	    iter = get_state_sub_list_iter(state, node->val.beta.a_store_no);
	    while(has_next_sub_list(iter)){
	      if(subs_equal_intersection(sub, get_next_sub_list(iter)))
		return;
	    }
	    insert_rete_beta_sub(state, node, node->children[0], copy_substitution(sub));
	  } else 
	  delete_substitution(sub);
      } else // right parent {
	if(insert_substitution(state, 
			       node->val.beta.a_store_no, 
			       sub, node->free_vars
			       ))
	  detract_rete_beta_sub(state, node->children[0], sub);
	else
	  delete_substitution(sub);
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
**/
void insert_rete_alpha_fact_children(rete_net_state* state, const rete_node* node, const atom* fact, substitution* sub){
  unsigned int i;
  assert(node->type == selector || node->type == alpha);
  for(i = 0; i < node->n_children; i++)
    insert_rete_alpha_fact(state, node->children[i], fact, copy_substitution(sub));
  //  delete_substitution(sub);
}

/**
   The substitution sub is a pointer to heap memory which will
   be deallocated or stored in a substitution list. The calling
   function must not touch sub after passing it to this function
**/
bool insert_rete_alpha_fact(rete_net_state* state, 
			    const rete_node* node, 
			    const atom* fact, 
			    substitution* sub)
{
  unsigned int i;
  const term *arg;
  sub_list_iter* iter;

  assert(test_substitution(sub));
  assert(test_term_list(fact->args));

  switch(node->type){
  case selector:
    assert(node->val.selector == fact->pred);
    insert_rete_alpha_fact_children(state, node, fact, sub);
    break;
  case alpha:
    assert(node->val.alpha.argument_no < fact->args->n_args);
    arg = fact->args->args[node->val.alpha.argument_no];
    assert(test_term(arg));
    if( ! unify_substitution_terms(arg, node->val.alpha.value, sub)){
      delete_substitution(sub);
      return false;
    }
#ifdef LAZY_RETE
    if(node->val.alpha.propagate){
#endif
      insert_rete_alpha_fact_children(state, node, fact, sub);
#ifdef LAZY_RETE
    } else {
      insert_in_sub_alpha_queue(state, node->axiom_no, sub, node);
    }
#endif
    break;
  case beta_and:
    assert(node->n_children == 1);
    if(!insert_substitution(state, node->val.beta.a_store_no, sub, node->free_vars)){  
      delete_substitution(sub);
      return false;
    }
    if(node->left_parent->type == beta_root){
      insert_rete_beta_sub(state, node, node->children[0], copy_substitution(sub));
    } else {
      iter = get_state_sub_list_iter(state, node->val.beta.b_store_no);
      while(has_next_sub_list(iter)){
	substitution* join = union_substitutions(get_next_sub_list(iter), sub);
	if(join != NULL) 
	  insert_rete_beta_sub(state, node, node->children[0], join);
      }
    }
    break;
  case beta_not: 
    if(insert_substitution(state, 
			   node->val.beta.a_store_no, 
			   sub, node->free_vars
			   ))
      detract_rete_beta_sub(state, node->children[0], sub);
    else {
      delete_substitution(sub);
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
  substitution* a = create_substitution(state->net->th, get_global_step_no(state));
  insert_rete_alpha_fact(state, sel, fact, a);
}
