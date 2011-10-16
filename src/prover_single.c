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

bool foundproof;





/**
   Wrappers for write_proof_node
**/

void print_state_single_rule_queues_rqs(rule_queue_state rqs, FILE* f){
  print_state_single_rule_queues(rqs.single, f);
}


void print_state_new_fact_store_rqs(rule_queue_state rqs, FILE* f){
  print_state_new_fact_store(rqs.single, f);
}

void write_prover_node_single(rete_state_single* state, rule_instance* next){
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
      insert_rete_net_fact_mt(state, ground, get_state_step_single(state));
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
bool return_reached_max_steps_mt(rete_state_single* state, rule_instance* ri){
  printf("Reached %i proof steps, higher than given maximum\n", get_state_step_single(state));
  free(ri);
  foundproof = false;
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
      
      rule_instance* next = choose_next_instance_single(state);
      if(next == NULL)
	return return_found_model_mt(state);
      
      
      bool incval = inc_proof_step_counter_single(state);
      if(!incval)
	return return_reached_max_steps_mt(state, next);
      
      //     insert_rule_instance_history_single(state, next);

      ts = get_state_step_single(state);



      if(next->rule->type == goal || next->rule->rhs->n_args == 0){
	//check_used_rule_instances_coq_single(next, state, ts, ts);
	//check_state_finished_single(state);
	write_prover_node_single(state, next);
	return true;
      } else {	// not goal rule
	if(next->rule->rhs->n_args > 1){
	  write_prover_node_single(state, next);
	  bool rv = start_rete_disjunction_coq_single(state, next, ts);
	  return rv;
	} else { // rhs is single conjunction
	  insert_rete_net_conjunction_single(state, next->rule->rhs->args[0], & next->sub);
	  write_prover_node_single(state, next);
	  //write_proof_edge(state, state);  
	}
      }
    } // end while(true)
 }

/**
   Called from the thread runner when a disjunction is popped which is treated for the second time.

   The disj_ri_stack_mutex MUST NOT be locked when this function is called.

   If net->treat_all_disjuncts is false, the first branch has already been run by start_rete_disjunction_coq_single below.
**/
bool start_rete_disjunction_coq_single(rete_state_single* state, rule_instance* next, unsigned int step){
  unsigned int i;
  rete_state_backup backup = backup_rete_state(state);
  for(i = 0; i < next->rule->rhs->n_args; i++){
    if(i > 0)
      state = restore_rete_state(&backup);
    bool rv;
    conjunction *con = next->rule->rhs->args[i];
    insert_rete_net_conjunction_single(state, con, & next->sub);
    rv = run_prover_single(state);
    if(!rv)
      return false;
    if(!next->used_in_proof)
      break;
  }
  destroy_rete_backup(&backup);

  return true;
}


/**
   The main prover function 
**/
unsigned int prover_single(const rete_net* rete, bool multithread){
  rete_state_single * state = create_rete_state_single(rete, verbose);
  const theory* th = rete->th;
  bool has_fact = false;
  unsigned int i, retval;
  atom * true_atom;

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
    insert_rete_net_fact_mt(state, true_atom);
  
  run_prover_single(state);

  if(foundproof && rete->coq){
    //write_single_coq_proof(state);
    //end_proof_coq_writer(rete->th);
  }
  retval = get_state_step_single(state);
  delete_rete_state_single(state);
  
  if(foundproof)
    return retval;
  else
    return 0;
}
