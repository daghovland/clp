/* prover_single.c

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
   An attempt at a simpler prover with no or-parallellism
**/

#include "common.h"
#include "theory.h"
#include "strategy.h"
#include "fresh_constants.h"
#include "rule_queue.h"
#include "rete.h"
#include "proof_writer.h"
#include "substitution.h"
#include "substitution_store.h"
#include "rule_instance_state_stack.h"
#include "rule_queue_single.h"
#include "rete_state_single.h"
#include "instantiate.h"
#include "rule_instance.h"
#include <errno.h>

extern bool debug, verbose;

bool foundproof, reached_max;





/**
   Wrappers for write_proof_node
**/

void print_state_single_rule_queues_rqs(rule_queue_state rqs, FILE* f){
  print_state_single_rule_queues(rqs.single, f);
}


void print_state_new_fact_store_rqs(rule_queue_state rqs, FILE* f){
  print_state_new_fact_store(rqs.single, f);
}

void write_prover_node_single(rete_state_single* state, const rule_instance* next){
  rule_queue_state rqs;
  rqs.single = state;
  write_proof_node(get_state_step_no_single(state), get_state_step_no_single(state), "N/A", state->net, rqs, state->constants, print_state_new_fact_store_rqs, print_state_single_rule_queues_rqs, next);
}




bool start_rete_disjunction_coq_single(rete_state_single* state, rule_instance* next, timestamp step);




void insert_rete_net_conjunction_single(rete_state_single* state, 
					clp_conjunction* con, 
					substitution* sub){
  unsigned int i;
  assert(test_conjunction(con, state->constants));
  assert(test_substitution(sub, state->constants));
  assert(test_is_conj_instantiation(con, sub, state->constants));
  for(i = 0; i < con->n_args; i++){
    bool fact_is_new = true;
    clp_atom* ground = instantiate_atom(con->args[i], sub, state->constants);
    assert(test_ground_atom(ground, state->constants));
#ifdef __DEBUG_RETE_STATE
    printf("New fact: ");
    print_fol_atom(ground, state->constants, stdout);
    printf("\n");
#endif
    if(ground->pred->is_equality){
      pause_rete_state_workers(state);
      union_constants(ground->args->args[0]->val.constant
		      , ground->args->args[1]->val.constant
		      , state->constants
		      , get_state_step_no_single(state)
		      , state->timestamp_store
		      );
#ifdef __DEBUG_RETE_STATE
      printf("New equality: Rechecking relevant parts of rete net.\n");
#endif
      recheck_rete_state_net(state);
    }
    else
      fact_is_new = insert_state_factset_single(state, ground);
    if(fact_is_new && (!state->net->factset_lhs || state->net->use_beta_not))
      insert_state_rete_net_fact(state, ground);
    if(!fact_is_new)
      delete_instantiated_atom(ground);
  } // end for
}

/**
   Called from run_prover_rete_coq when not finding new instance
**/
bool return_found_model_mt(rete_state_single* state){
  fprintf(stdout, "Found a model of the theory \n");
  print_all_constants(state->constants, stdout);
  print_state_fact_store(state, stdout);
  foundproof = false;
  return false;
}

/**
   Called from run_prover_rete_coq when reaching max steps, as given with the 
   commandline option -m|--max=LIMIT
**/
bool return_reached_max_steps_mt(rete_state_single* state, const rule_instance* ri){
  printf("Reached %i proof steps, higher than given maximum\n", get_state_step_no_single(state));
  foundproof = false;
  reached_max = true;
  return false;
}


/**
   Runs a branch in the proof.
   Should be thread-safe. 
   Note that the rule instance on the stack is discarded, as the 
   conjunction is already inserted by insert_rete_net_disjunction_coq_single below
**/

bool run_prover_single(rete_state_single* state){
    while(true){
      timestamp ts;
      rule_instance* next;
      bool incval;
      assert(test_rete_state(state));
      next = choose_next_instance_single(state);
      assert(test_rete_state(state));
      if(next == NULL)
	return return_found_model_mt(state);
      fresh_exist_constants(next->rule, & next->sub, state->constants);
      assert(test_rule_instance(next, state->constants));
      incval = inc_proof_step_counter_single(state);
      if(!incval)
	return return_reached_max_steps_mt(state, next);
      ts = create_normal_timestamp(get_state_step_no_single(state));
      if(next->rule->type == goal || next->rule->rhs->n_args == 0){
	end_proof_branch(state->current_proof_branch, ts, 0);
	check_used_rule_instances_coq_single(next, state, state->current_proof_branch, ts, ts);
	write_prover_node_single(state, next);
	return true;
      } else {	// not goal rule
	if(next->rule->rhs->n_args > 1){
	  end_proof_branch(state->current_proof_branch, ts, next->rule->rhs->n_args);
	  write_prover_node_single(state, next);
	  bool rv = start_rete_disjunction_coq_single(state, next, ts);
	  return rv;
	} else { // rhs is single conjunction
	  write_prover_node_single(state, next);
	  insert_rete_net_conjunction_single(state, next->rule->rhs->args[0], & next->sub);
	  write_proof_edge(state->current_proof_branch->name, state->cur_step, state->current_proof_branch->name, state->cur_step);  
	}
      }
    } // end while(true)
 }

/**
   Called from the thread runner when a disjunction is popped which is treated for the second time.

   The disj_ri_stack_mutex MUST NOT be locked when this function is called.

   If net->treat_all_disjuncts is false, the first branch has already been run by start_rete_disjunction_coq_single below.

   Note that the rule instance pointer next may be invalidated during every branch, 
   this is why the next and sub is reinstantiated at every loop iteration
**/
bool start_rete_disjunction_coq_single(rete_state_single* state, rule_instance* next, timestamp step){
  unsigned int i;
  const clp_axiom* rule = next->rule;
  unsigned int n_branches = rule->rhs->n_args;
  rete_state_backup backup = backup_rete_state(state);
  proof_branch* parent_prf_branch = state->current_proof_branch;
  for(i = 0; i < n_branches; i++){
    bool rv;
    const substitution * sub;
    substitution *tmp_sub;
    if(i > 0)
      restore_rete_state(&backup, state);
    enter_proof_disjunct(state);
    clp_conjunction *con = rule->rhs->args[i]; 
    sub = & (get_historic_rule_instance(state, step.step))->sub;
    tmp_sub = copy_substitution(sub, & state->tmp_subs, state->net->th->sub_size_info, state->timestamp_store, state->constants);
    assert(test_substitution(tmp_sub, state->constants));
    insert_rete_net_conjunction_single(state, con, tmp_sub);
    write_proof_edge(parent_prf_branch->name, step.step, state->current_proof_branch->name, state->cur_step);
    rv = run_prover_single(state);
    if(!rv)
      return false;
    if(!state->net->treat_all_disjuncts && !(get_historic_rule_instance(state, step.step))->used_in_proof){
      restore_rete_state(&backup, state);
      prune_proof(state->current_proof_branch, i);
      break;
    }
  }
  destroy_rete_backup(&backup);
    
  return true;
}
/**
   Auxiliary for write_single_coq_prof
   Instantiates "get_history" in proof_writer
**/
rule_instance* get_history_single(unsigned int no, rule_queue_state rqs){
  return get_historic_rule_instance(rqs.single, no);
}

/**
   Writes the coq proof after the prover is done.
   Used by the multithreaded version
**/
void write_single_coq_proof(rete_state_single* state, proof_branch* branch){
  FILE* coq_fp = get_coq_fdes();
  timestamp step = branch->end_step;
  rule_instance* end_ri = get_historic_rule_instance(state, step.step);
  const clp_axiom* rule = end_ri->rule;
  unsigned int n_branches = branch->n_children;
  timestamp step_ri, pusher;
  rule_queue_state rqs;
  rqs.single = state;
  assert( n_branches == 0 || n_branches == rule->rhs->n_args);
  init_rev_stack(branch->elim_stack);
  fprintf(coq_fp, "(* Treating branch %s *)\n", branch->name);
  assert(is_empty_ri_stack(branch->elim_stack) || ! is_empty_rev_ri_stack(branch->elim_stack));
  while(!is_empty_rev_ri_stack(branch->elim_stack)){
    rule_instance* ri = get_historic_rule_instance(state, pop_rev_ri_stack(branch->elim_stack, &step_ri, &pusher).step);
    write_elim_usage_proof(state->net, ri, step_ri, state->constants);
  }
  if(rule->type == goal && n_branches <= 1){
    fprintf(coq_fp, "(* Reached leaf at step %i *)\n", step.step);
    write_goal_proof(end_ri, state->net, step, get_history_single, rqs, state->constants);
    fprintf(coq_fp, "(* Finished goal proof of leaf at step %i *)\n", step.step);

  } else {
    unsigned int i;
    assert(n_branches > 1);
    write_elim_usage_proof(state->net, end_ri, step, state->constants);
    for(i = 0; i < n_branches; i++){
      assert(branch != branch->children[i]);
      if(i > 0)
	write_disj_proof_start(end_ri, step, i, state->constants);
      write_single_coq_proof(state, branch->children[i]);
    }
    if(is_fact(end_ri->rule)){
      fprintf(coq_fp, "(* disjunction at %i is a fact, no lhs to prove *)\n", step.step);
    } else {
      fprintf(coq_fp, "(* Proving lhs of disjunction at %i *)\n", step.step);
      write_premiss_proof(end_ri, step, state->net, get_history_single, rqs, state->constants);
      fprintf(coq_fp, "(* Finished proof of lhs of step %i *)\n", step.step);
    }
  }
  while(!is_empty_ri_stack(branch->elim_stack)){
    rule_instance* ri = get_historic_rule_instance(state, pop_ri_stack(branch->elim_stack, &step_ri, &pusher).step);
    if(!is_fact(ri->rule)){
      fprintf(coq_fp, "(* Proving lhs of existential at %i (Used by step %i)*)\n", step_ri.step, pusher.step);
      write_premiss_proof(ri, step_ri, state->net, get_history_single, rqs, state->constants);
      fprintf(coq_fp, "(* Finished proving lhs of existential at %i *)\n", step_ri.step);
    }
  }
  fprintf(coq_fp, "(* Finished with branch %s *)\n", branch->name);
}


/**
   The main prover function 
**/
unsigned int prover_single(const rete_net* rete, bool multithread){
  rete_state_single * state = create_rete_state_single(rete, verbose);
  bool has_fact = false;
  unsigned int i, retval;
  clp_atom * true_atom;
  foundproof = true;
  reached_max = false;
  srand(1000);
  for(i = 0; i < rete->th->n_init_model; i++){
    assert(!rete->th->init_model[i]->is_existential);
    has_fact = true;
    substitution* sub = create_empty_substitution(rete->th, & state->tmp_subs);
    insert_rete_net_conjunction_single(state, rete->th->init_model[i], sub);
  }
  if(rete->th->n_init_model == 0){
    for(i = 0; i < rete->th->n_axioms; i++){
      const clp_axiom * ax = rete->th->axioms[i];
      if(ax->type == fact){
	has_fact = true;
	break;
      }
    }
    if(!has_fact){
      printf("The theory has no facts, hence the empty model satisifies it.\n");
      return 0;
    }
  }
  true_atom = create_prop_variable("true", (theory*) rete->th);
  insert_state_factset_single(state, true_atom);
  if(!state->net->factset_lhs || state->net->use_beta_not)
    insert_state_rete_net_fact(state, true_atom);
  run_prover_single(state);
  stop_rete_state_single(state);
  if(foundproof && rete->coq && !reached_max){
    write_single_coq_proof(state, state->root_branch);
    end_proof_coq_writer(rete->th);
  }
  retval = get_state_total_steps(state);
  delete_rete_state_single(state);
  
  if(foundproof && !reached_max)
    return retval;
  else
    return 0;
}
