/* prover.c

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
#include "theory.h"
#include "strategy.h"
#include "fresh_constants.h"
#include "rule_queue.h"
#include "rete.h"
#include "proof_writer.h"
#include "substitution.h"
#include "rule_instance_state_stack.h"
#ifdef HAVE_PTHREAD
#include <pthread.h>
#endif
#include <errno.h>

extern bool debug, verbose;

bool foundproof;

#ifdef HAVE_PTHREAD
unsigned int num_threads;
#define MAX_THREADS 20

pthread_mutex_t prover_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t prover_cond = PTHREAD_COND_INITIALIZER;
#endif

/*
  A stack of used, but not proved disjunctive rules

  Pushed in "check_used_rule_instance" and popped in run_prover

  initialized by prover()
**/
rule_instance_state_stack* disj_ri_stack;

rule_instance_state** history;
size_t size_history;

bool insert_rete_net_disjunction(rete_net_state*, rule_instance*, bool);
bool insert_rete_disjunction_coq_mt(rete_net_state* state, rule_instance* next);

#ifdef HAVE_PTHREAD
/**
   Used by pthread error and pthread cancel
**/
void pthread_error_test(int retno, const char* msg){
  if(retno != 0){
    errno = retno;
    perror(msg);
    exit(EXIT_FAILURE);
  }
}
#endif

/**
   Called at the end of a disjunctive branch. 
   Checks what rule instances were used

**/
void check_used_rule_instances(rule_instance* ri, rete_net_state* historic_state, rete_net_state* current_state, unsigned int historic_ts, unsigned int current_ts){
  unsigned int i;
  if((!ri->used_in_proof 
      || (ri->rule->rhs->n_args == 1 && ! ri->rule->is_existential)  ) 
     && (ri->rule->type != fact || ri->rule->rhs->n_args > 1)){
    unsigned int n_premises = ri->substitution->n_timestamps;
    ri->used_in_proof = true;
    for(i = 0; i < n_premises; i++){
      int premiss_no = ri->substitution->timestamps[i];
      assert(premiss_no == history[premiss_no]->step_no);
      if(premiss_no > 0)
	check_used_rule_instances((history[premiss_no])->ri, (history[premiss_no])->s, current_state, (history[premiss_no])->step_no, premiss_no);
    }
    // The stack is popped again in prover.c after the disjunction is finished
    if(ri->rule->rhs->n_args > 1)
      push_ri_state_stack(disj_ri_stack, ri, historic_state, historic_ts);
    else if(ri->rule->rhs->n_args == 1 && ri->rule->rhs->args[0]->is_existential){
      push_ri_state_stack(disj_ri_stack, ri, historic_state, current_ts);
    }
      
    write_elim_usage_proof(historic_state, ri, current_ts);
  }
}

/**
   Called at the end of a disjunctive branch
   Checks what rule instances were used
   Called from run_prover_rete_coq_mt
**/
void check_used_rule_instances_coq_mt(rule_instance* ri, rete_net_state* historic_state, unsigned int historic_ts){
  unsigned int i;
  if(!ri->used_in_proof && (ri->rule->type != fact || ri->rule->rhs->n_args > 1)){
    unsigned int n_premises = ri->substitution->n_timestamps;
    ri->used_in_proof = true;
    for(i = 0; i < n_premises; i++){
      int premiss_no = ri->substitution->timestamps[i];
      assert(premiss_no == history[premiss_no]->step_no);
      check_used_rule_instances_coq_mt((history[premiss_no])->ri, (history[premiss_no])->s, (history[premiss_no])->step_no);
    }
    if(ri->rule->rhs->n_args > 1)
      ;
    else if(ri->rule->rhs->n_args == 1 && ri->rule->rhs->args[0]->is_existential)
      push_ri_stack(historic_state->elim_stack, ri, historic_ts);
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
  if(step >= net->size_history){
    pt_err(pthread_mutex_lock(&history_array_lock), "Could not get lock on history array.\n");
#endif
    while(step >= size_history - 1){
      size_history *= 2;
      history = realloc_tester(history, size_history * sizeof(rule_instance*));
    }
#ifdef HAVE_PTHREAD
    pt_err(pthread_mutex_unlock(&history_array_lock), "Could not release lock on history array.\n");
  } // end second if, avoiding unnecessary locking
#endif
  /**
     Note that s->cursteps is set by inc_proof_step_counter to the current global step that is worked on
  **/
  history[step] = create_rule_instance_state(ri, s, get_current_state_step_no(s));
}

void insert_rete_net_conjunction(rete_net_state* state, 
				 conjunction* con, 
				 substitution* sub, 
				 bool factset){
  unsigned int i, j;

  
  assert(test_conjunction(con));
  assert(test_substitution(sub));

  if(!state->net->existdom)
    fresh_exist_constants(state, con, sub);
  
  assert(test_is_conj_instantiation(con, sub));
  
  for(i = 0; i < con->n_args; i++){
    atom* ground = instantiate_atom(con->args[i], sub);
    
    assert(test_ground_atom(ground));
    
    if(ground->pred->is_domain){
      assert(ground->args->n_args == 1);
      assert(ground->args->args[0]->type != variable_term);
      insert_domain_elem(state, ground->args->args[0]);
    }
    
#ifdef __DEBUG_RETE_STATE
    printf("New fact: ");
    print_fol_atom(ground, stdout);
    printf("\n");
#endif
    
    if(factset)
      insert_state_fact_set(state, ground);
    else {
      insert_rete_net_fact(state, ground);
    }
    delete_instantiated_atom(con->args[i], ground);
  } // end for
}

/**
   Called from run_prover_rete_coq when not finding new instance
**/
bool return_found_model(rete_net_state* state){
  fprintf(stdout, "Found a model of the theory: \n");
  print_fact_set(state->facts, stdout);
  foundproof = false;
  return false;
}

/**
   Called from run_prover_rete_coq when not finding new instance
**/
bool return_reached_max_steps(rete_net_state* state, rule_instance* ri){
  printf("Reached %i proof steps, higher than given maximum\n", get_latest_global_step_no(state));
  free(ri);
  foundproof = false;
  return false;
}

/**
   A new prover for coq output without proof rewriting

   The goal is to make this multithreaded

   Note that the rule instance on the stack is discarded, as the 
   conjunction is already inserted by insert_rete_net_disjunction_coq_mt below
**/

bool run_prover_rete_coq_mt(rete_net_state* state){
    while(true){
      rule_instance* next = choose_next_instance(state, state->net->strat);
      if(next == NULL)
	return return_found_model(state);
      
      assert(next->rule->type != fact || next->rule->rhs->n_args != 1);
      
      bool incval = inc_proof_step_counter(state);
      if(!incval)
	return return_reached_max_steps(state, next);
      
      insert_rule_instance_history(state, next);
      write_proof_node(state, next);
      
      if(next->rule->type == goal || next->rule->rhs->n_args == 0){
	unsigned int ts = get_current_state_step_no(state);
	check_used_rule_instances_coq_mt(next, state, ts);
	state->end_of_branch = next;
	break;
      } else {	// not goal rule
	if(next->rule->rhs->n_args > 1){
	  bool insval = insert_rete_disjunction_coq_mt(state, next);
	  if(!insval)
	    return false;
	  break;
	} else { // rhs is single conjunction
	  assert(next->rule->type != fact);
	  insert_rete_net_conjunction(state, next->rule->rhs->args[0], next->substitution, true);
	}
      }
    } // end while(true)
 }


/**
   One instance of this function is run in each thread
**/
void thread_runner_coq_rete(void){
  while(!is_empty_ri_state_stack(disj_ri_stack)){
    rete_net_state* state; 
    rule_instance* next;
    unsigned int step;
    bool rp_val;
    pop_ri_state_stack(disj_ri_stack, &next, &state, &step);
    rp_val = run_prover_rete_coq_mt(state);
    if(!rp_val)
      return;
  } // end while(stack_of_disjunctions_not_empty)
}


/**
   Auxiliary function called from insert_rete_net_disjunction_coq_mt
**/
rete_net_state* run_rete_proof_disj_branch(rete_net_state* state, conjunction* con, substitution* sub, unsigned int branch_no){
  rete_net_state* copy_state = split_rete_state(state, branch_no);
  write_proof_edge(state, copy_state);
  insert_rete_net_conjunction(copy_state, con, sub, true);
  return copy_state;
}


bool insert_rete_disjunction_coq_mt(rete_net_state* state, rule_instance* next){
  const axiom* rule;
  unsigned int i;
  unsigned int step;
  bool retval;
  rete_net_state* copy_state;
  bool rp_val;

  assert(rule->rhs->n_args > 1);

  rule = next->rule;
  step = get_current_state_step_no(state);
  copy_state = run_rete_proof_disj_branch(state, rule->rhs->args[0], next->substitution, 0);
  rp_val  = run_prover_rete_coq_mt(copy_state);
  if(!retval)
    return false;

  if(!next->used_in_proof){
    state->end_of_branch = copy_state->end_of_branch;
    add_ri_stack(state->elim_stack, copy_state->elim_stack);
    return true;
  }
  for(i = 1; i < rule->rhs->n_args; i++){
    copy_state = run_rete_proof_disj_branch(state, rule->rhs->args[i], next->substitution, i);
    push_ri_state_stack(disj_ri_stack, NULL, copy_state, step);
  }
  state->end_of_branch = next;
  return true;
}


/**
   The main loop in a single-threaded prover

   Uses a RETE state, but does not allocate or delete it

   Uses rewriting of the proof: A disjunction is moved closest to the goal where it is used

   Not really working
**/

bool run_prover(rete_net_state* state, bool factset, rule_instance* stack_marker){
  bool pop_disj_stack = false;
  rete_net_state* copy_state;
  rule_instance* next;
  unsigned int ts, start_step;
  start_step = get_current_state_step_no(state);
  while(true){
    while(pop_disj_stack){
      bool retval, found_new_disj = false;
      while(!is_empty_ri_state_stack(disj_ri_stack)){
	pop_ri_state_stack(disj_ri_stack, &next, &state, &ts);

	if(next == stack_marker){
	  // This means the branch is finished
	  assert(next->rule == NULL);
	  delete_dummy_rule_instance(next);
	  return true;
	}
	if(next->rule->rhs->n_args > 1){
	  if(state->net->coq)
	    fprintf(get_coq_fdes(), "(* Popped split %i from disj_ri_stack *)\n", ts);
	  found_new_disj = true;
	  break;
	}
	assert(next->rule->rhs->n_args == 1);
	if(state->net->coq){
	  fprintf(get_coq_fdes(), "(* Proving eliminated instance from step %i :", ts);
	  print_coq_rule_instance(next, get_coq_fdes());
	  fprintf(get_coq_fdes(), " *)\n");
	  
	  write_premiss_proof(next, ts, history);
	  fprintf(get_coq_fdes(), "(* Finished proof of eliminated instance from step %i *)\n", ts);
	}
	next->used_in_proof = false;
      }
      if(!found_new_disj)
	return true;
      if(!insert_rete_net_disjunction(state, next, factset))
	return false;
      found_new_disj = false;
    } // end if(pop_disj_stack)
    if(factset)
      next = factset_next_instance(state->net->th, state->facts);
    else
      next = choose_next_instance(state, state->net->strat);
    if (next == NULL)
      break;
    
    assert(test_rule_instance(next, state));
    
    if(next->rule->type != fact || next->rule->rhs->n_args != 1){
      if(!inc_proof_step_counter(state)){
	printf("Reached %i proof steps, higher than given maximum\n", get_latest_global_step_no(state));
	free(next);
	return false;
      }
      insert_rule_instance_history(state, next);
      write_proof_node(state, next);
      
      if(next->rule->type == goal || next->rule->rhs->n_args == 0){
	unsigned int ts = get_current_state_step_no(state);
	check_used_rule_instances(next, state, state, ts, ts);
	if(state->net->coq){
	  fprintf(get_coq_fdes(), "(* Reached leaf at step %i *)\n", ts);
	  write_goal_proof(next, state,  ts, history);
	  fprintf(get_coq_fdes(), "(* Finished goal proof of leaf at step %i *)\n", ts);
	}
	pop_disj_stack = true;
	continue;
      }
    }
    if(next->rule->rhs->n_args > 1)
      copy_state = split_rete_state(state, 0);
    else
      copy_state = state;
    if(next->rule->type != fact)
      write_proof_edge(state, copy_state);
    state = copy_state;
    insert_rete_net_conjunction(state, next->rule->rhs->args[0], next->substitution, factset);
  } // end while(true)
  fprintf(stdout, "Found a model of the theory: \n");
  print_fact_set(state->facts, stdout);
  return false;
}  

/**
   Inserting a disjunctive rule into a network single threaded

   Creates and deletes new states for each thread

   Note that goal rules should never be inserted (This does not make sense, as no facts would be inserted)

  **/
bool insert_rete_net_disjunction(rete_net_state* state, rule_instance* ri, bool factset){
  unsigned int i, j;
  const disjunction* dis = ri->rule->rhs;
  bool thread_can_run;
  unsigned int step = get_current_state_step_no(state);

  assert(test_rule_instance(ri, state));
  assert(dis->n_args > 0);

  rete_net_state* copy_states[dis->n_args];

  if(state->net->coq)
    fprintf(get_coq_fdes(), "(* Started properly treating split at step %i *)\n", step);

  //check_used_rule_instances(ri, state, state, step, step);

  for(i = 1; i < dis->n_args; i++){
    rule_instance* tmp = create_dummy_rule_instance();

    copy_states[i] = split_rete_state(state, i);
    write_proof_edge(state, copy_states[i]);
    insert_rete_net_conjunction(copy_states[i], dis->args[i], ri->substitution, factset);
    
    if(i > 0 && state->net->coq){
      write_disj_proof_start(ri, step, i);
    }
    push_ri_state_stack(disj_ri_stack, tmp, state, step);
    
    if(!run_prover(copy_states[i], factset, tmp)){
      delete_rete_state(copy_states[i], state);
      return false;
    }
    //delete_rete_state(copy_states[i], state);
    
    /*
    if(!ri->used_in_proof){
      printf("Disjunction in step %i not used, so skipping rest of branches. This should not happen now, indicates error in prover. Exiting.\n", step);
      assert(false);
      return false;
      } */

  }
  
  if(state->net->coq){
    fprintf(get_coq_fdes(), "(* Finished branch at step %i *)\n", step);
    fprintf(get_coq_fdes(), "(* Proving premises of step %i *)\n", step);
    write_premiss_proof(ri, step, history);
    fprintf(get_coq_fdes(), "(* Finished proof of premises of step %i *)\n", step);	
  }
  // The next line is necessary in the cases where a disjunction is used several places
  /*if(state->net->coq)
    ri->used_in_proof = false;*/

  return true;
}
/**
   The main prover function

   Creates and deletes a RETE state on the heap
   
   If factset is true, it does not use a rete network

   Returns 0 if no proof found, otherwise, the number of steps
**/
unsigned int prover(const rete_net* rete, bool factset, bool multithread){
  unsigned int i, j, retval;
  rete_net_state* state = create_rete_state(rete, verbose);
  const theory* th = rete->th;

#ifdef HAVE_PTHREAD
  num_threads = 0;
#endif

  disj_ri_stack = initialize_ri_state_stack();
  size_history = 5;
  history = calloc_tester(sizeof(rule_instance_state*), size_history);


  srand(1000);
  for(i=0; i < th->n_axioms; i++){
    if(th->axioms[i]->type == fact && th->axioms[i]->rhs->n_args == 1){
      const axiom* axm = th->axioms[i];
      substitution* sub = create_substitution(th, get_current_state_step_no(state));
      assert(axm->axiom_no == i);
      insert_rete_net_conjunction(state, axm->rhs->args[0], sub, factset);
      insert_rule_instance_history(state, create_rule_instance(axm,sub));
      inc_proof_step_counter(state);
    }
  }
  if(multithread){
    push_ri_state_stack(disj_ri_stack, NULL, state, get_current_state_step_no(state));
    foundproof = true;
    thread_runner_coq_rete();
  } else 
    foundproof =  run_prover(state, factset, NULL);
  retval = get_latest_global_step_no(state);
  delete_full_rete_state(state);
  delete_ri_state_stack(disj_ri_stack);
  if(foundproof)
    return retval;
  else
    return 0;
}
