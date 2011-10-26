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
  write_proof_node(get_state_step_no_single(state), get_state_step_no_single(state), "N/A", state->net, rqs, print_state_new_fact_store_rqs, print_state_single_rule_queues_rqs, next);
}




bool start_rete_disjunction_coq_single(rete_state_single* state, rule_instance* next, unsigned int step);




void insert_rete_net_conjunction_single(rete_state_single* state, 
					conjunction* con, 
					substitution* sub){
  unsigned int i, j;
  assert(test_conjunction(con));
  assert(test_substitution(sub));
  fresh_exist_constants(con, sub, & state->constants);
  assert(test_is_conj_instantiation(con, sub));
  for(i = 0; i < con->n_args; i++){
    atom* ground = instantiate_atom(con->args[i], sub);
    assert(test_ground_atom(ground));
#ifdef __DEBUG_RETE_STATE
    printf("New fact: ");
    print_fol_atom(ground, stdout);
    printf("\n");
#endif
    if(!state->net->factset_lhs || state->net->use_beta_not)
      insert_state_rete_net_fact(state, ground, get_state_step_single(state));
    if(state->net->has_factset)
      insert_state_factset_single(state, ground);
    else 
      delete_instantiated_atom(con->args[i], ground);
  } // end for
}

/**
   Called from run_prover_rete_coq when not finding new instance
**/
bool return_found_model_mt(rete_state_single* state){
  fprintf(stdout, "Found a model of the theory \n");
  if(state->net->has_factset)
    print_state_fact_store(state, stdout);
  else
    fprintf(stdout, "Rerun with \"--print_model\" to show the model.\n");
  foundproof = false;
  return false;
}

/**
   Called from run_prover_rete_coq when reaching max steps, as given with the 
   commandline option -m|--max=LIMIT
**/
bool return_reached_max_steps_mt(rete_state_single* state, const rule_instance* ri){
  printf("Reached %i proof steps, higher than given maximum\n", get_state_step_single(state));
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
      unsigned int i, ts;
      rule_instance* next;
      bool incval;
      next = choose_next_instance_single(state);
      if(next == NULL)
	return return_found_model_mt(state);
      assert(test_rule_instance(next));
      incval = inc_proof_step_counter_single(state);
      if(!incval)
	return return_reached_max_steps_mt(state, next);
      ts = get_state_step_single(state);
      if(next->rule->type == goal || next->rule->rhs->n_args == 0){
	check_used_rule_instances_coq_single(next, state, ts, ts);
	write_prover_node_single(state, next);
	return true;
      } else {	// not goal rule
	if(next->rule->rhs->n_args > 1){
	  write_prover_node_single(state, next);
	  bool rv = start_rete_disjunction_coq_single(state, next, ts);
	  return rv;
	} else { // rhs is single conjunction
	  write_prover_node_single(state, next);
	  insert_rete_net_conjunction_single(state, next->rule->rhs->args[0], & next->sub);
	  //write_proof_edge(state, state);  
	}
      }
    } // end while(true)
 }

/**
   Called from the thread runner when a disjunction is popped which is treated for the second time.

   The disj_ri_stack_mutex MUST NOT be locked when this function is called.

   If net->treat_all_disjuncts is false, the first branch has already been run by start_rete_disjunction_coq_single below.

   Note that the rule instance pointer next may be invalidated during this run
**/
bool start_rete_disjunction_coq_single(rete_state_single* state, rule_instance* next, unsigned int step){
  unsigned int i;
  const axiom* rule = next->rule;
  unsigned int n_branches = rule->rhs->n_args;
  substitution *sub = copy_substitution(&next->sub, &state->tmp_subs, state->net->th->sub_size_info);
  rete_state_backup backup = backup_rete_state(state);
  for(i = 0; i < n_branches; i++){
    bool rv;
    if(i > 0)
      state = restore_rete_state(&backup);
    conjunction *con = rule->rhs->args[i];
    insert_rete_net_conjunction_single(state, con, sub);
    rv = run_prover_single(state);
    if(!rv)
      return false;
    if(!(get_historic_rule_instance(state, step))->used_in_proof){
      //      fprintf(stdout, "Could eliminate other branches of step %i\n", step);
      break;
    }
  }
  free_substitution(sub);
  destroy_rete_backup(&backup);

  return true;
}

#if SINGLE_WRITES_PROOF
/**
   Writes the coq proof after the prover is done.
   Used by the multithreaded version
**/
void write_single_coq_proof(rete_state_single* state){
  FILE* coq_fp = get_coq_fdes();
  unsigned int n_branches = state->end_of_branch->rule->rhs->n_args;
  unsigned int step = get_current_state_step_no(state);
  unsigned int step_ri, pusher;
  init_rev_stack(state->elim_stack);
  fprintf(coq_fp, "(* Treating branch %s *)\n", state->proof_branch_id);
  assert(is_empty_ri_stack(state->elim_stack) || ! is_empty_rev_ri_stack(state->elim_stack));
  while(!is_empty_rev_ri_stack(state->elim_stack)){
    rule_instance* ri = pop_rev_ri_stack(state->elim_stack, &step_ri, &pusher);
    write_elim_usage_proof(state, ri, step_ri);
  }
  if(state->end_of_branch->rule->type == goal && n_branches <= 1){
    fprintf(coq_fp, "(* Reached leaf at step %i *)\n", step);
    write_goal_proof(state->end_of_branch, state, step, history);
    fprintf(coq_fp, "(* Finished goal proof of leaf at step %i *)\n", step);

  } else {
    unsigned int i;
    assert(n_branches > 1);
    write_elim_usage_proof(state, state->end_of_branch, step);
    for(i = 0; i < n_branches; i++){
      assert(state != state->branches[i]);
      if(i > 0)
	write_disj_proof_start(state->end_of_branch, step, i);
      write_mt_coq_proof(state->branches[i]);
    }
    fprintf(coq_fp, "(* Proving lhs of disjunction at %i *)\n", step);
    write_premiss_proof(state->end_of_branch, step, history);
    fprintf(coq_fp, "(* Finished proof of lhs of step %i *)\n", step);
  }
  while(!is_empty_ri_stack(state->elim_stack)){
    rule_instance* ri = pop_ri_stack(state->elim_stack, &step_ri, &pusher);
    fprintf(coq_fp, "(* Proving lhs of existential at %i (Used by step %i)*)\n", step_ri, pusher);
    write_premiss_proof(ri, step_ri, history);
    fprintf(coq_fp, "(* Finished proving lhs of existential at %i *)\n", step_ri);
  }
  fprintf(coq_fp, "(* Finished with branch %s *)\n", state->proof_branch_id);
}
#endif
/**
   The main prover function 
**/
unsigned int prover_single(const rete_net* rete, bool multithread){
  rete_state_single * state = create_rete_state_single(rete, verbose);
  const theory* th = rete->th;
  bool has_fact = false;
  unsigned int i, retval;
  atom * true_atom;
  foundproof = true;
  reached_max = false;

  srand(1000);
  
  for(i = 0; i < rete->th->n_axioms; i++){
    if(rete->th->axioms[i]->type == fact){
      has_fact = true;
      break;
    }
  }
  if(!has_fact){
    printf("The theory has no facts, and is therefore never contradictory.\n");
    return 0;
  }
  
  true_atom = create_prop_variable("true", (theory*) rete->th);
  if(state->net->has_factset)
    insert_state_factset_single(state, true_atom);

  if(!state->net->factset_lhs || state->net->use_beta_not)
    insert_state_rete_net_fact(state, true_atom);
  
  run_prover_single(state);

  if(foundproof && rete->coq && !reached_max){
    //write_single_coq_proof(state);
    end_proof_coq_writer(rete->th);
  }
  retval = get_state_step_single(state);
  delete_rete_state_single(state);
  
  if(foundproof && !reached_max)
    return retval;
  else
    return 0;
}
