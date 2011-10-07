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
#include "substitution_memory.h"
#include "rule_instance_state_stack.h"
#include <errno.h>

extern bool debug, verbose;

bool foundproof;




rule_instance_state_stack* disj_ri_stack;

rule_instance_state * * history;
size_t size_history;

bool start_rete_disjunction_coq_single(rete_net_state_single* state, rule_instance* next, unsigned int step);



/**
   Called at the end of a disjunctive branch
   Checks what rule instances were used
   Called from run_prover_rete_coq_single

   At the moment, all rule instances are unique in eqch branch (they are copied when popped from the queue)
   So we know that changing the "used_in_proof" here is ok.
**/
void check_used_rule_instances_coq_single(rule_instance* ri, rete_net_state* historic_state, unsigned int historic_ts, unsigned int current_ts){
  unsigned int i;
  assert(ri == history[historic_ts]->ri);
  if(!ri->used_in_proof && (ri->rule->type != fact || ri->rule->rhs->n_args > 1)){
    unsigned int n_premises = ri->substitution->n_timestamps;
    if(ri->rule->rhs->n_args <= 1){
      ri = copy_rule_instance(ri);
      history[historic_ts]->ri = ri;
    }
    ri->used_in_proof = true;
    for(i = 0; i < n_premises; i++){
      int premiss_no = ri->substitution->timestamps[i];
      if(premiss_no > 0){
	assert(premiss_no == history[premiss_no]->step_no);
	check_used_rule_instances_coq_single((history[premiss_no])->ri, (history[premiss_no])->s, (history[premiss_no])->step_no, current_ts);
      }
    }
    if(ri->rule->rhs->n_args == 1 && ri->rule->rhs->args[0]->is_existential)
      push_ri_stack(historic_state->elim_stack, ri, historic_ts, current_ts);
  }
}


/**
   Create rule instance history. Called from run_prover
   Used by check_used_rule_instances and coq proof output
**/
void insert_rule_instance_history(rete_net_state* s, rule_instance* ri){
  rete_net* net = (rete_net*) s->net;
  unsigned int step = get_current_state_step_no(s);
#ifdef HAVE_PTHREAD
  if(step >= size_history -  1){
    pt_err(pthread_mutex_lock(history_mutex), "Could not get lock on history array.\n");
#endif
    while(step >= size_history - 1){
      unsigned int i;
      size_history *= 2;
      history = realloc_tester(history, size_history * sizeof(rule_instance*));
#ifndef NDEBUG
      for(i = size_history / 2; i < size_history; i++)
	history[i] = NULL;
#endif
      
    }
#ifdef HAVE_PTHREAD
    pt_err(pthread_mutex_unlock(history_mutex), "Could not release lock on history array.\n");
  } // end second if, avoiding unnecessary locking
#endif
  /**
     Note that s->cursteps is set by inc_proof_step_counter to the current global step that is worked on
  **/
  assert(history[step] == NULL);
  history[step] = create_rule_instance_state(ri, s, get_current_state_step_no(s));
}

void insert_rete_net_conjunction_single(rete_net_state* state, 
					conjunction* con, 
					substitution* sub){
  unsigned int i, j;
  assert(test_conjunction(con));
  assert(test_substitution(sub));

  fresh_exist_constants(state, con, sub);
  
  assert(test_is_conj_instantiation(con, sub));
  
  for(i = 0; i < con->n_args; i++){
    atom* ground = instantiate_atom(con->args[i], sub);
    
    assert(test_ground_atom(ground));
    
    
#ifdef __DEBUG_RETE_STATE
    printf("New fact: ");
    print_fol_atom(ground, stdout);
    printf("\n");
#endif
    
    if(state->net->has_factset){
      insert_state_fact_set(state, ground);
    }
    if(!state->net->factset_lhs && state->net->use_beta_not)
      insert_rete_net_fact(state, ground);
  } // end for
}

/**
   Called from run_prover_rete_coq when not finding new instance
**/
bool return_found_model(rete_net_state* state){
  fprintf(stdout, "Found a model of the theory \n");
  if(state->net->has_factset)
    print_state_fact_set(state, stdout);
  else
    fprintf(stdout, "Rerun with \"--print_model\" to show the model.\n");
  foundproof = false;
#ifdef HAVE_PTHREAD
  pthread_cond_broadcast(&prover_done_cond);
  pthread_cond_broadcast(& stack_increased_cond);
#endif
  return false;
}

/**
   Called from run_prover_rete_coq when reaching max steps, as given with the 
   commandline option -m|--max=LIMIT
**/
bool return_reached_max_steps(rete_net_state* state, rule_instance* ri){
  printf("Reached %i proof steps, higher than given maximum\n", get_latest_global_step_no(state));
  free(ri);
  foundproof = false;
#ifdef HAVE_PTHREAD
  pthread_cond_broadcast(&prover_done_cond);
  pthread_cond_broadcast(& stack_increased_cond);
#endif
  return false;
}

/**
   Checks parent states as "finished" 
   Called when reaching goal state in run_prover_rete_coq_single
   The finished is read from insert_rete_disjunction_coq_single
**/
void check_state_finished(rete_net_state* state){
  rete_net_state* parent = (rete_net_state*) state->parent;
  unsigned int n_branches;
  bool found_self = false;
  unsigned int i;
  if (state->parent == NULL){
    assert(state->start_step_no == 0);
    return;
  }
  n_branches = parent->end_of_branch->rule->rhs->n_args;
  state->finished = true;
  for(i = 0; i < n_branches; i++){
    if(!parent->branches[i]->finished)
      return;
    if(state == parent->branches[i]){
      if(i==0)
	return;
      found_self = true;
    }
  }
  assert(found_self);
  check_state_finished(parent);
}

/**
   Runs a branch in the proof.

   Should be thread-safe. 

   Note that the rule instance on the stack is discarded, as the 
   conjunction is already inserted by insert_rete_net_disjunction_coq_single below
**/

bool run_prover_single(rete_net_state* state){
    while(true){
      unsigned int i, ts;
      
      rule_instance* next = choose_next_instance(state, state->net->strat);
      if(next == NULL)
	return return_found_model(state);
      
      
      bool incval = inc_proof_step_counter(state);
      if(!incval)
	return return_reached_max_steps(state, next);
      
      insert_rule_instance_history(state, next);

      ts = get_current_state_step_no(state);



      if(next->rule->type == goal || next->rule->rhs->n_args == 0){
	check_used_rule_instances_coq_single(next, state, ts, ts);
	check_state_finished(state);
	write_proof_node(state, next);
	return true;
      } else {	// not goal rule
	if(next->rule->rhs->n_args > 1){
	  write_proof_node(state, next);
	  bool rv = start_rete_disjunction_coq_single(state, next);
	  return rv;
	} else { // rhs is single conjunction
	  insert_rete_net_conjunction(state, next->rule->rhs->args[0], next->substitution);
	  write_proof_node(state, next);
	  write_proof_edge(state, state);  
	}
      }
    } // end while(true)
 }

/**
   Called from the thread runner when a disjunction is popped which is treated for the second time.

   The disj_ri_stack_mutex MUST NOT be locked when this function is called.

   If net->treat_all_disjuncts is false, the first branch has already been run by start_rete_disjunction_coq_single below.
**/
bool start_rete_disjunction_coq_single(rete_net_state_single* state, rule_instance* next, unsigned int step){
  unsigned int i;
  rete_state_backup rsb = backup_rete_state(state);
  for(i = 0; i < next->rule->rhs->n_args; i++){
    bool rv;
    conjunction *con = next->rule->rhs->args[i];
    insert_rete_net_conjunction(state, con, sub);
    rv = run_prover_single(state);
    if(!rv)
      return false;
    

  return;
}


/**
   The main prover function 
**/
unsigned int prover_single(const rete_net* rete, bool multithread){
  rete_net_state_single* state = create_rete_state_single(rete, verbose);
  const theory* th = rete->th;

  size_history = 5;
  history = calloc_tester(size_history, sizeof(rule_instance_state*));


  srand(1000);
  
  has_fact = false;
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
    insert_state_fact_set(state, true_atom);

  if(!state->net->factset_lhs && state->net->use_beta_not)
    insert_rete_net_fact(state, true_atom);
  
  run_prover_single(state);

  if(foundproof && rete->coq){
    write_single_coq_proof(state);
    end_proof_coq_writer(rete->th);
  }
  retval = get_latest_global_step_no(state);
  delete_rete_state_single(state);
  
  if(foundproof)
    return retval;
  else
    return 0;
}
