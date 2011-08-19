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
rule_instance* normal_next_instance(rete_net_state* state){
  unsigned int i;
  const theory* th = state->net->th;
  bool has_definite = false;
  bool has_definite_non_splitting = false;
  bool has_non_splitting = false;
  bool has_next_rule = false;
  bool may_have_next_rule = false;
  size_t definite_non_splitting_rule, definite_rule, non_splitting_rule;
  size_t lightest_rule = th->n_axioms;
  unsigned int min_weight = 2 * get_global_step_no(state) * (1 + RAND_RULE_WEIGHT);
  unsigned int axiom_weights[th->n_axioms];
  unsigned int max_weight = 2 * (get_global_step_no(state)+1) * (1 +  RAND_RULE_WEIGHT);
  rule_instance* retval;

  for(i = 0; i < th->n_axioms; i++){
    const axiom* rule = th->axioms[i];
    size_t axiom_no = rule->axiom_no;
    assert(axiom_no == i);
    if(rule->type == fact && rule->rhs->n_args == 1)
      continue;
    if(axiom_may_have_new_instance(axiom_no, state)){
      unsigned int rule_previously_applied = state->axiom_inst_queue[i]->previous_appl;
      axiom_weights[axiom_no] = (rule_queue_possible_age(axiom_no, state) + rule_previously_applied) * (1 + rand() / RAND_DIV);
      
      may_have_next_rule = true;
      
      if( (rule->type == goal || rule->type == fact) && axiom_has_new_instance(axiom_no, state))
	return pop_axiom_rule_queue(state, axiom_no);
      
      if(!rule->is_existential && !rule->has_domain_pred && axiom_has_new_instance(axiom_no, state)){
	has_definite = true;
	definite_rule = axiom_no;
	if(rule->rhs->n_args == 1){
	  has_definite_non_splitting = true;
	  definite_non_splitting_rule = axiom_no;
	}
      } else { // not definite rule
	if(rule->rhs->n_args == 1){
	  has_non_splitting = true;
	  non_splitting_rule = axiom_no;
	  axiom_weights[axiom_no] /= 20;
	}
	if(axiom_weights[axiom_no] < min_weight){
	  min_weight = axiom_weights[axiom_no];
	  lightest_rule = axiom_no;
	}
	assert(min_weight <  max_weight);
      } // end not definite rule

    } // end if rule queue not empty
    else 
      axiom_weights[axiom_no] = max_weight;
  } // end for all axioms

  if(has_definite_non_splitting)
    return pop_axiom_rule_queue(state, definite_non_splitting_rule);

  if(has_definite)
    return pop_axiom_rule_queue(state, definite_rule);

  while(may_have_next_rule){
    if(may_have_next_rule && axiom_has_new_instance(lightest_rule, state))
      return pop_axiom_rule_queue(state, lightest_rule);
    axiom_weights[lightest_rule] = max_weight;
    may_have_next_rule = false;
    for(i = 0; i < th->n_axioms; i++){
      if(axiom_weights[i] < axiom_weights[lightest_rule]){
	lightest_rule = i;
	may_have_next_rule = true;
      }
    }
  }
  return NULL;
}
  
/**
   This strategy chooser tries to emulate CL.pl
**/
rule_instance* clpl_next_instance(rete_net_state* state){
  unsigned int i;
  const theory* th = state->net->th;
  for(i = 0; i < th->n_axioms; i++){
    size_t axiom_no = th->axioms[i]->axiom_no;
    assert(i == axiom_no);
    if(axiom_has_new_instance(axiom_no, state)){
      rule_instance* retval;
      retval = pop_axiom_rule_queue(state, axiom_no);
      return retval;
    }
  } // end for i -- n_axioms
  return NULL;
}


rule_instance* factset_next_instance(const theory* th, const fact_set* fs){
  return create_rule_instance(th->axioms[0], create_substitution(th, 1));
}

/**
   Returns next rule instance, or NULL if there is no more instances
**/
rule_instance* choose_next_instance(rete_net_state* state, strategy strat){
  switch(strat){
  case clpl_strategy:
    return clpl_next_instance(state);
  case normal_strategy:
    return normal_next_instance(state);
  default:
    fprintf(stderr, "Unknown strategy %i\n", strat);
    exit(EXIT_FAILURE);
  }
}
