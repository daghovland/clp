/* strategy.c

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
#include "strategy.h"
#include "rete.h"
#include "rete_net.h"
#include "theory.h"
#include "rule_queue.h"
#include "sub_alpha_queue.h"
#include "fresh_constants.h"
#include "proof_writer.h"

// Used by normal_next_instance()
#define RAND_RULE_WEIGHT 100
#define  RAND_DIV (RAND_MAX / RAND_RULE_WEIGHT)
		 

/**
   This is where the strategy is chosen

   The strategy here, is to first take any facts or goal rules

   Then take any without disjunction or exist. quantifiers, then 
   any without exist. quantifiers.

   If no such rule is in the queue, there is a rotation based on
   state->next
**/
rule_instance* normal_next_instance(rule_queue_state state
					 , const rete_net* net
					 , unsigned int step_no
					 , bool (*is_empty) (rule_queue_state, size_t)
					 , rule_instance* (*peek_axiom)(rule_queue_state, size_t)
					 , bool (*has_new_instance)(rule_queue_state, size_t)
					 , unsigned int (*possible_age)(rule_queue_state, size_t)
					 , bool (*may_have)(rule_queue_state, size_t)
					 , rule_instance* (*pop_axiom)(rule_queue_state, size_t)
					 , void (*add_to_queue) (const axiom*, const substitution*, rule_queue_state)
					 , unsigned int (*previous_application)(rule_queue_state, size_t)
					 )
{
  unsigned int i;
  const theory* th = net->th;
  bool has_definite = false;
  bool has_definite_non_splitting = false;
  bool has_non_splitting = false;
  bool has_next_rule = false;
  bool may_have_next_rule = false;
  size_t definite_non_splitting_rule, definite_rule, non_splitting_rule;
  size_t lightest_rule = th->n_axioms;
  unsigned int min_weight = 2 * step_no * (1 + RAND_RULE_WEIGHT);
  unsigned int axiom_weights[th->n_axioms];
  unsigned int max_weight = 40 * step_no * (1 + RAND_RULE_WEIGHT);

  rule_instance* retval;

    
 
  for(i = 0; i < th->n_axioms; i++){
    const axiom* rule = th->axioms[i];
    size_t axiom_no = rule->axiom_no;
    assert(axiom_no == i);
#if false
    if(net->factset_lhs){
      substitution* sub;
      bool found_new_instance = false;
      if(rule->type == fact && !is_empty(state, axiom_no))
	continue;
      while(!found_new_instance && !is_empty(state, axiom_no)){
	rule_instance* ri = peek_axiom(state, axiom_no);
	if(disjunction_true_in_fact_set(factset, net->th->axioms[axiom_no]->rhs, &ri->sub))
	  ri = pop_axiom(state, axiom_no);
	else
	  found_new_instance = true;
      }
      if(!found_new_instance){
	if(false_in_fact_set(factset, axiom_no, & sub))
	  add_to_queue(rule, sub, state);
	else
	  continue;
      }
    }
#endif
    if(may_have(state, axiom_no)){
      unsigned int rule_previously_applied = previous_application(state, i);
      if( (rule->type == goal || rule->type == fact)){
	if (has_new_instance(state, axiom_no))
	  return pop_axiom(state, axiom_no);
	else {
	  axiom_weights[axiom_no] = max_weight;
	  continue;
	}
      }
      axiom_weights[axiom_no] = 
	(possible_age(state, axiom_no) + rule_previously_applied) 
	* rule->lhs->n_args 
	* (1 + rand() / RAND_DIV);
      
           
      if(!rule->is_existential){
	if(has_new_instance(state, axiom_no)){
	  may_have_next_rule = true;
	  has_definite = true;
	  definite_rule = axiom_no;
	  if(rule->rhs->n_args == 1){
	    has_definite_non_splitting = true;
	    definite_non_splitting_rule = axiom_no;
	    return pop_axiom(state, definite_non_splitting_rule);
	  }
	} else {
	  axiom_weights[axiom_no] = max_weight;
	  continue;
	}
      } else { // not definite rule
	may_have_next_rule = true;
	if(rule->rhs->n_args == 1){
	  has_non_splitting = true;
	  non_splitting_rule = axiom_no;
	  axiom_weights[axiom_no] /= 20;
	}
	if(axiom_weights[axiom_no] < min_weight){
	  min_weight = axiom_weights[axiom_no];
	  lightest_rule = axiom_no;
	}
	if(axiom_weights[axiom_no] >= max_weight){
	  max_weight = axiom_weights[axiom_no];
	}
	assert(min_weight <=  max_weight);
      } // end not definite rule

    } // end if rule queue not empty
    else 
      axiom_weights[axiom_no] = max_weight;
  } // end for all axioms

  if(has_definite_non_splitting)
    return pop_axiom(state, definite_non_splitting_rule);

  if(has_definite)
    return pop_axiom(state, definite_rule);

  while(may_have_next_rule){
    assert(min_weight <= max_weight && axiom_weights[lightest_rule] == min_weight);
    if(may_have_next_rule && has_new_instance(state, lightest_rule))
      return pop_axiom(state, lightest_rule);
    axiom_weights[lightest_rule] = max_weight;
    min_weight = max_weight;
    may_have_next_rule = false;
    for(i = 0; i < th->n_axioms; i++){
      if(axiom_weights[i] < min_weight){
	lightest_rule = i;
	min_weight = axiom_weights[i];
	may_have_next_rule = true;
      }
      if(axiom_weights[i] > max_weight)
	max_weight = axiom_weights[i];
    }
  }
  return NULL;
}
  
/**
   This strategy chooser tries to emulate CL.pl
**/
rule_instance* clpl_next_instance(rule_queue_state state
				       , const rete_net* net
				       , bool (*has_new_instance)(rule_queue_state, size_t)
				       , rule_instance* (*pop_axiom)(rule_queue_state, size_t)
				       )
{
  unsigned int i;
  rule_instance* retval;
  const theory* th = net->th;
  for(i = 0; i < th->n_axioms; i++){
    size_t axiom_no = th->axioms[i]->axiom_no;
    assert(i == axiom_no);
    if(has_new_instance(state, axiom_no)){
      retval = pop_axiom(state, axiom_no);
      return retval;
    }
  } // end for i -- n_axioms
  return NULL;
}



/**
   Returns next rule instance, or NULL if there is no more instances
**/
rule_instance* choose_next_instance(rule_queue_state state
				    , const rete_net* net
				    , strategy strat
				    , unsigned int step_no
				    , bool (*is_empty) (rule_queue_state, size_t)
				    , rule_instance* (*peek_axiom)(rule_queue_state, size_t)
				    , bool (*has_new_instance)(rule_queue_state, size_t)
				    , unsigned int (*possible_age)(rule_queue_state, size_t)
				    , bool (*may_have)(rule_queue_state, size_t)
				    , rule_instance* (*pop_axiom)(rule_queue_state, size_t)
				    , void (*add_to_queue) (const axiom*, const substitution*, rule_queue_state)
				    , unsigned int (*previous_application)(rule_queue_state, size_t)
				    )
{
  switch(strat){
  case clpl_strategy:
    return clpl_next_instance(state, net, has_new_instance, pop_axiom);
  case normal_strategy:
    return normal_next_instance(state, net, step_no, is_empty, peek_axiom, has_new_instance, possible_age, may_have, pop_axiom, add_to_queue, previous_application);
  default:
    fprintf(stderr, "Unknown strategy %i\n", strat);
    exit(EXIT_FAILURE);
  }
}

