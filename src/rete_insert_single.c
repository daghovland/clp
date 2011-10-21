/* rete_insert_single.c

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
   Code for updating state of single-threaded rete network
**/
#include "common.h"
#include "term.h"
#include "fresh_constants.h"
#include "rete.h"
#include "rete_insert_single.h"
#include "substitution.h"

/**
   Inserting a substitution into a beta node in a rete network
   The substitution sub is changed, but not freed
   The calling function must not use the values in sub, and must free sub after the call returns
**/
void insert_rete_beta_sub_single(const rete_net* net,
				 substitution_store_array * node_caches, 
				 substitution_store_mt * tmp_subs,
				 rule_queue_single* rule_queue, 
				 const rete_node* parent, 
				 const rete_node* node, 
				 unsigned int step, 
				 const substitution* sub)
{
  sub_store_iter iter;
  substitution_size_info ssi = net->th->sub_size_info;
  substitution * tmp_sub = create_empty_substitution(net->th, tmp_subs);

  assert(parent == node->left_parent || parent == node->val.beta.right_parent);
  assert(test_substitution(sub));
  
  if(node->type == rule){
    assert(node->n_children == 0);
    assert(test_is_instantiation(node->val.rule.axm->rhs->free_vars, sub));
    assert(test_substitution(sub));	
    if(insert_substitution_single(node_caches, node->val.rule.store_no, sub, node->free_vars)){
      push_rule_instance_single(rule_queue
				, node->val.rule.axm
				, sub
				, step
				, net->strat == clpl_strategy
				);
      //      add_rule_to_queue_single(node->val.rule.axm, sub, rqs);
    }
  } else {

    assert(node->type == beta_and);
    assert(node->n_children == 1);	
    assert(node->left_parent != NULL && node->left_parent->type != alpha);
    
    
    switch(node->type){
    case beta_and:
      if(insert_substitution_single(node_caches, 
				    node->val.beta.b_store_no, 
				    sub, node->free_vars
				    ))
	{
	  iter = get_array_sub_store_iter(node_caches, node->val.beta.a_store_no);
	  while(has_next_sub_store(& iter)){
	    bool overlapping_subs = false;
	    if(node->in_positive_lhs_part)
	      overlapping_subs = union_substitutions_struct_with_ts(tmp_sub, sub, get_next_sub_store(&iter), ssi);
	    else
	      overlapping_subs = union_substitutions_struct_one_ts(tmp_sub, sub, get_next_sub_store(&iter), ssi);
	    if(overlapping_subs) 
	      insert_rete_beta_sub_single(net, node_caches, tmp_subs, rule_queue,  node, node->children[0], step, tmp_sub);
	  } // end while has next ..
	  destroy_sub_store_iter(& iter);
	} // end if insert ..
      break;
#if false
    case beta_not:
      if(parent == node->left_parent){
	if(insert_substitution_single(node_caches, 
				      node->val.beta.b_store_no, 
				      sub, node->free_vars
				      ))
	  {
	    bool found_overlapping_sub = false;
	    iter = get_array_sub_store_iter(node_caches, node->val.beta.a_store_no);
	    while(has_next_sub_store(& iter)){
	      if(subs_equal_intersection(sub, get_next_sub_store(& iter))){
		found_overlapping_sub = true;
		break;
	      }
	    }
	    destroy_sub_store_iter(& iter);
	    if(!found_overlapping_sub)
	      insert_rete_beta_sub_single(net->th, node_caches, tmp_subs,  node, node->children[0], step, sub);
	  } // end if(insert_sub...
      } else // right parent 
	if(insert_substitution_single(node_caches, 
				      node->val.beta.a_store_no, 
				      sub, node->free_vars
				      ))
	  ;
      break;
#endif
    default:
      fprintf(stderr, "Encountered untreated node type: %i\n", node->type);
      assert(false);
    } // end switch (node->type) 
  } /* end if(node->type == rule) ... else ... */
  free_substitution(tmp_sub);
  return;
}

/**
   Inserting fact into all alpha children 
   Called from insert_rete_alpha_fact on selectors and alpha nodes
   
   The substitution is changed, but not deleted, the calling function must deallocate it and not depend on the contents after calling
**/
void insert_rete_alpha_fact_children_single(const rete_net* net,
					    substitution_store_array * node_caches, 
					    substitution_store_mt * tmp_subs,
					    rule_queue_single * rule_queue,
					    const rete_node* node, 
					    const atom* fact, 
					    unsigned int step, 
					    substitution* sub){
  unsigned int i;
  substitution* tmp_sub = create_empty_substitution(net->th, tmp_subs);
  assert(node->type == selector || node->type == alpha);
  for(i = 0; i < node->n_children; i++){
    copy_substitution_struct(tmp_sub, sub, net->th->sub_size_info);
    insert_rete_alpha_fact_single(net, node_caches, tmp_subs,  rule_queue, node->children[i], fact, step, tmp_sub);
  }
  free_substitution(tmp_sub);
}

/**
   The substitution sub is a pointer to heap memory which will
   be deallocated or stored in a substitution list. The calling
   function must not touch sub after passing it to this function, as it
   is stored or deleted as it is.

   propagate is true when call originates from axiom_has_new_instance in lazy_rule_queue.c, otherwise false.
   true propagate overrides the value of node->propagate
**/
bool insert_rete_alpha_fact_single(const rete_net* net,
				   substitution_store_array * node_caches, 
				   substitution_store_mt * tmp_subs,
				   rule_queue_single * rule_queue,
				   const rete_node* node, 
				   const atom* fact, 
				   unsigned int step, 
				   substitution* sub)
{
  unsigned int i;
  const term *arg;
  sub_store_iter iter;
  substitution_size_info ssi = net->th->sub_size_info;
  substitution* tmp_sub = create_empty_substitution(net->th, tmp_subs);
  bool ret_val = true;

  assert(test_substitution(sub));

  switch(node->type){
  case selector:
    assert(false);
    break;
  case alpha:
    assert(node->val.alpha.argument_no < fact->args->n_args);
    arg = fact->args->args[node->val.alpha.argument_no];
    assert(test_term(arg));
    if( ! unify_substitution_terms(arg, node->val.alpha.value, sub))
      ret_val = false;
    else
      insert_rete_alpha_fact_children_single(net, node_caches, tmp_subs, rule_queue, node, fact, step, sub);
    break;
  case beta_and:
    assert(node->n_children == 1);
    if(!insert_substitution_single(node_caches, node->val.beta.a_store_no, sub, node->free_vars))
      ret_val = false;
    else {
      if(node->left_parent->type == beta_root)
	insert_rete_beta_sub_single(net, node_caches, tmp_subs, rule_queue, node, node->children[0], step, sub);
      else {
	iter = get_array_sub_store_iter(node_caches, node->val.beta.b_store_no);
	while(has_next_sub_store(& iter)){
	  bool has_overlap;
	  if(node->in_positive_lhs_part)
	    has_overlap = union_substitutions_struct_with_ts(tmp_sub, get_next_sub_store(& iter), sub, ssi);
	  else
	    has_overlap = union_substitutions_struct_one_ts(tmp_sub, get_next_sub_store(& iter), sub, ssi);
	  if(has_overlap) 
	    insert_rete_beta_sub_single(net, node_caches, tmp_subs, rule_queue, node, node->children[0], step, tmp_sub);
	}
	destroy_sub_store_iter(& iter);
      }
    }
    break;
  case beta_not: 
    if(insert_substitution_single(node_caches, 
				  node->val.beta.a_store_no, 
				  sub, node->free_vars
				  ))
      ;
    else 
      ret_val = false;
    break;
  default:
    fprintf(stderr, "Encountered wrong node type %i\n", node->type);
    assert(false);
  }
  free_substitution(tmp_sub);
  return ret_val;
}    

