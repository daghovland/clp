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
#include "timestamps.h"
#include "logger.h"

/**
   Should test that the timestamps actually refer to rules 
   that introduced the facts nevessary for the instances.
   TODO: Not finished.
**/
bool test_rule_substitution(const clp_axiom* rule, const substitution* sub, const constants* cs){
  unsigned int i = 0;
  timestamps_iter iter = get_timestamps_iter(& sub->sub_ts);
  while(has_next_timestamps_iter(&iter)){
    assert(i < rule->lhs->n_args);
    timestamp ts = get_next_timestamps_iter(&iter);
    const clp_term* val = get_sub_value(sub,i);
    if(val != NULL && !test_term(val, cs))
      return false;
    if(ts.step < 0) 
      return false;
    ++i;
  }
  destroy_timestamps_iter(&iter);
  return true;
}

/**
   Gets a constant, either the one in the clp_term *t, or the
   substitution value of the variable.
   Called from insert below
**/
dom_elem get_instantiated_constant(const clp_term* t, const substitution* sub, const constants* cs){
  if(t->type == variable_term)
    t = find_substitution(sub, t->val.var, cs);
  assert(t->type == constant_term);
  return t->val.constant;
}



/**
   Inserting a substitution into a beta node in a rete network
   The substitution sub is changed, but not freed
   The calling function must not use the values in sub, and must free sub after the call returns

   exit_matcher usually points to the component stop_worker of a rete_worker_thread
**/
void insert_rete_beta_sub_single(const rete_net* net,
				 substitution_store_array * node_caches, 
				 substitution_store_mt * tmp_subs,
				 timestamp_store* ts_store, 
				 rule_queue_single* rule_queue, 
				 const rete_node* parent, 
				 const rete_node* node, 
				 unsigned int step, 
				 substitution* sub,
				 constants* cs
				 )
{
  sub_store_iter iter;
  dom_elem c1, c2;
  const clp_term *t1, *t2;
  substitution_size_info ssi = net->th->sub_size_info;
  substitution * tmp_sub = create_empty_substitution(net->th, tmp_subs);

  assert(parent == node->left_parent || parent == node->val.beta.right_parent);
  assert(test_substitution(sub, cs));
  
  //assert(node->rule_no < net->th->n_axioms && test_timestamps(net->th->axioms[node->rule_no], sub));
  
  if(node->type == rule_node){
    assert(node->n_children == 0);
    //    assert(test_is_instantiation(node->val.rule.axm->rhs->free_vars, sub));
    assert(test_substitution(sub, cs));
#ifdef DEBUG_RETE_INSERT
    FILE* out = get_logger_out(__FILE__, __LINE__);
    fprintf(out, "\nInserting ");
    print_substitution(sub, cs, out);
    fprintf(out, "into rule node for axiom %i", node->rule_no);
    fprintf(out, "\n");
    finished_logging(__FILE__, __LINE__);
#endif	
    if(insert_substitution_single(node_caches, node->val.rule.store_no, sub, node->free_vars, cs, ts_store)){
      push_rule_instance_single(rule_queue
				, node->val.rule.axm
				, sub
				, step
				, net->strat == clpl_strategy
				, ts_store
				, cs
				);
      //      add_rule_to_queue_single(node->val.rule.axm, sub, rqs);
#if false
      printf("Inserted ");
      print_substitution(sub, cs, stdout);
      printf("into rule node for axiom %i", node->rule_no);
      printf("\n");
#endif	
    }
  } else {

    assert(node->type == beta_and || node->type == equality_node || node->type == beta_not);
    assert(node->n_children == 1);	
    assert(node->left_parent != NULL && node->left_parent->type != alpha);
    switch(node->type){
    case equality_node:
      t1 = node->val.equality.t1;
      c1 = get_instantiated_constant(t1, sub, cs);
      t2 = node->val.equality.t2;
      c2 = get_instantiated_constant(t2, sub, cs);
      add_reflexivity_timestamp(&sub->sub_ts, step, ts_store);
      if(insert_substitution_single(node_caches, 
				    node->val.equality.b_store_no, 
				    sub, node->free_vars, cs, 
				    ts_store
				    ))
	{
	  if(equal_constants_mt(c1, c2, cs, &sub->sub_ts, ts_store, true))
	    insert_rete_beta_sub_single(net, node_caches, tmp_subs, ts_store, rule_queue, node, node->children[0], step, sub, cs);
	}
      break;
    case beta_and:
      if(insert_substitution_single(node_caches, 
				    node->val.beta.b_store_no, 
				    sub, node->free_vars, cs, 
				    ts_store
				    ))
	{
#ifdef DEBUG_RETE_INSERT
	  FILE* out = get_logger_out(__FILE__,__LINE__);
	  fprintf(out, "Inserting ");
	  print_substitution(sub, cs, out);
	  fprintf(out, "into beta store %i for axiom %i", node->val.beta.b_store_no, node->rule_no);
	  fprintf(out, "\n");
	  finished_logging(__FILE__, __LINE__);
#endif	
	  iter = get_array_sub_store_iter(node_caches, node->val.beta.a_store_no);
	  while(has_next_sub_store(& iter)){
	    bool overlapping_subs = false;
	    if(node->in_positive_lhs_part)
	      overlapping_subs = union_substitutions_struct_with_ts(tmp_sub, sub, get_next_sub_store(&iter), ssi, cs, ts_store);
	    else
	      overlapping_subs = union_substitutions_struct_one_ts(tmp_sub, sub, get_next_sub_store(&iter), ssi, cs, ts_store);
	    if(overlapping_subs) 
	      insert_rete_beta_sub_single(net, node_caches, tmp_subs, ts_store, rule_queue,  node, node->children[0], step, tmp_sub, cs);
	  } // end while has next ..
	  destroy_sub_store_iter(& iter);
	} // end if insert ..
      break;
    case beta_not:
      if(parent == node->left_parent){
	if(
	   insert_substitution_single(node_caches, 
				      node->val.beta.b_store_no, 
				      sub, node->free_vars, cs, 
				      ts_store
				      ))
	  {
	    bool found_overlapping_sub = false;
	    iter = get_array_sub_store_iter(node_caches, node->val.beta.a_store_no);
	    while(has_next_sub_store(& iter)){
	      if(subs_equal_intersection(sub, get_next_sub_store(& iter), cs)){
		found_overlapping_sub = true;
		break;
	      }
	    }
	    destroy_sub_store_iter(& iter);
	    if(!found_overlapping_sub)
	      insert_rete_beta_sub_single(net, node_caches, tmp_subs,  ts_store, rule_queue, node, node->children[0], step, sub, cs);
	  } // end if(insert_sub...
      } else // right parent 
	if(insert_substitution_single(node_caches, 
				      node->val.beta.a_store_no, 
				      sub, node->free_vars, cs, 
				      ts_store
				      ))
	  ;
      break;
    default:
      fprintf(stderr, "Encountered untreated node type: %i\n", node->type);
      assert(false);
    } // end switch (node->type) 
  } /* end if(node->type == rule) ... else ... */
  free_substitution(tmp_sub);
  return;
}



/**
   Called from recheck_beta_node
   and from rete_insert_alpha

   Inserts from the alpha into the beta node
**/
void insert_rete_alpha_beta(const rete_net* net,
			    substitution_store_array * node_caches, 
			    substitution_store_mt * tmp_subs,
			    timestamp_store* ts_store, 
			    const rete_node* node, 
			    rule_queue_single * rule_queue,
			    unsigned int step, 
			    substitution* sub, 
			    constants* cs)
{
  if(node->left_parent->type == beta_root)
    insert_rete_beta_sub_single(net, node_caches, tmp_subs, ts_store, rule_queue, node, node->children[0], step, sub, cs);
  else {
    substitution* tmp_sub = create_empty_substitution(net->th, tmp_subs);
    substitution_size_info ssi = net->th->sub_size_info;
    sub_store_iter iter = get_array_sub_store_iter(node_caches, node->val.beta.b_store_no);
    while(has_next_sub_store(& iter)){
      bool has_overlap;
      if(node->in_positive_lhs_part)
	has_overlap = union_substitutions_struct_with_ts(tmp_sub, get_next_sub_store(& iter), sub, ssi, cs, ts_store);
      else
	has_overlap = union_substitutions_struct_one_ts(tmp_sub, get_next_sub_store(& iter), sub, ssi, cs, ts_store);
      if(has_overlap) 
	insert_rete_beta_sub_single(net, node_caches, tmp_subs, ts_store, rule_queue, node, node->children[0], step, tmp_sub, cs);
    }
    destroy_sub_store_iter(& iter);
    free_substitution(tmp_sub);
  }
}

/**
   Called when a new equality is added to the model. 
   For each beta node, first re-inserts all beta into the next beta-node, 
   then reinserts all from the alpha node into the beta node.

   Should first be called at the rule node, iterates backwards
**/
void recheck_beta_node(const rete_net* net,
		       substitution_store_array * node_caches, 
		       substitution_store_mt * tmp_subs,
		       timestamp_store* ts_store, 
		       const rete_node* node, 
		       rule_queue_single * rule_queue,
		       unsigned int step, 
		       constants* cs)
{
  sub_store_iter iter;
  substitution* tmp_sub;
  switch(node->type){
  case beta_root:
    return;
  case equality_node:
    recheck_beta_node(net, node_caches, tmp_subs, ts_store, node->left_parent, rule_queue, step, cs);
    break;
  case beta_not:
    recheck_beta_node(net, node_caches, tmp_subs, ts_store, node->val.beta.right_parent, rule_queue, step, cs);
    break;
  case beta_and:
    tmp_sub = create_empty_substitution(net->th, tmp_subs);
    iter= get_array_sub_store_iter(node_caches, node->val.beta.a_store_no);
    while(has_next_sub_store(& iter)){
      copy_substitution_struct(tmp_sub, get_next_sub_store(&iter), net->th->sub_size_info, ts_store, false, cs);
      insert_rete_alpha_beta(net, node_caches, tmp_subs, ts_store, node, rule_queue, step, tmp_sub, cs);
    }
    destroy_sub_store_iter(& iter);
    break;
  case rule_node:
    break;
  default:
    fprintf(stderr, "%s, line %i recheck_beta_node: Invalid node type.\n", __FILE__, __LINE__);
    assert(false);
    exit(EXIT_FAILURE);
  }
  if(node->type != equality_node)
    recheck_beta_node(net, node_caches, tmp_subs, ts_store, node->left_parent, rule_queue, step, cs);
}
/**
   Inserting fact into all alpha children 
   Called from insert_rete_alpha_fact on selectors and alpha nodes
   
   The substitution is changed, but not deleted, the calling function must deallocate it and not depend on the contents after calling

   exit_matcher usually points to the component stop_worker of a rete_worker_thread

   Returns false if the fact is not passed into beta network in all children
**/
bool insert_rete_alpha_fact_children_single(const rete_net* net,
					    substitution_store_array * node_caches, 
					    substitution_store_mt * tmp_subs,
					    timestamp_store* ts_store, 
					    rule_queue_single * rule_queue,
					    const rete_node* node, 
					    const clp_atom* fact, 
					    unsigned int step, 
					    substitution* sub, 
					    constants* cs){
  unsigned int i;
  bool ret_val = true;
  substitution* tmp_sub = create_empty_substitution(net->th, tmp_subs);
  assert(node->type == selector_node || node->type == alpha);
  for(i = 0; i < node->n_children; i++){
    copy_substitution_struct(tmp_sub, sub, net->th->sub_size_info, ts_store, false, cs);
    if(!insert_rete_alpha_fact_single(net, node_caches, tmp_subs,  ts_store, rule_queue, node->children[i], fact, step, tmp_sub, cs))
      ret_val = false;
  }
  free_substitution(tmp_sub);
  return ret_val;
}

/**
   The substitution sub is a pointer to heap memory which will
   be deallocated or stored in a substitution list. The calling
   function must not touch sub after passing it to this function, as it
   is modified and then stored or deleted

   propagate is true when call originates from axiom_has_new_instance in lazy_rule_queue.c, otherwise false.
   true propagate overrides the value of node->propagate

   exit_matcher usually points to the component stop_worker of a rete_worker_thread

   fact is not changed. It is assumed that the factset will contain this, and it is destroyed
   at the end of the prover.

   Returns true if the fact reaches the beta nodes. Returns false if the substitution is rejected by the alpha 
   network. This is necessary so the rete worker can store the facts that must be reinserted when new equalities
   are encountered
**/
bool insert_rete_alpha_fact_single(const rete_net* net,
				   substitution_store_array * node_caches, 
				   substitution_store_mt * tmp_subs,
				   timestamp_store * ts_store,  
				   rule_queue_single * rule_queue,
				   const rete_node* node, 
				   const clp_atom* fact, 
				   unsigned int step, 
				   substitution* sub, 
				   constants* cs)
{
  const clp_term *arg;
  bool ret_val = true;
  assert(test_substitution(sub, cs));
  switch(node->type){
  case selector_node:
    assert(false);
    break;
  case alpha:
    assert(node->val.alpha.argument_no < fact->args->n_args);
    arg = fact->args->args[node->val.alpha.argument_no];
    assert(test_term(arg, cs));
    if( ! unify_substitution_terms(arg, node->val.alpha.value, sub, cs, ts_store))
      ret_val = false;
    else
      ret_val = insert_rete_alpha_fact_children_single(net, node_caches, tmp_subs, ts_store, rule_queue, node, fact, step, sub, cs);
    break;
  case beta_and:
    ret_val = true;
    assert(node->n_children == 1);
    if(insert_substitution_single(node_caches, node->val.beta.a_store_no, sub, node->free_vars, cs, ts_store))
      insert_rete_alpha_beta(net, node_caches, tmp_subs, ts_store, node, rule_queue, step, sub, cs);
    break;
  case beta_not: 
    ret_val = true;
    insert_substitution_single(node_caches, 
			       node->val.beta.a_store_no, 
			       sub, node->free_vars, cs, 
			       ts_store
			       );
    break;
  default:
    fprintf(stderr, "Encountered wrong node type %i\n", node->type);
    assert(false);
  }
  return ret_val;
}    

