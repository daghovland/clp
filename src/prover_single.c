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

bool insert_rete_net_disjunction_coq_single(rete_net_state_single*, rule_instance*, bool);
bool start_rete_disjunction_coq_single(rete_net_state_single* state, rule_instance* next);
void insert_rete_disjunction_coq_single(rete_net_state_single* state, rule_instance* next, unsigned int step);



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

void insert_rete_net_conjunction(rete_net_state* state, 
				 conjunction* con, 
				 substitution* sub){
  unsigned int i, j;
  assert(test_conjunction(con));
  assert(test_substitution(sub));

  // At the moment the substitution is copied, when the instance is popped, 
  // so we know it is unique. This could be change if memory becomes an issue.
  if(!state->net->existdom)
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

    /*    if(!state->net->has_factset)
	  delete_instantiated_atom(con->args[i], ground);*/
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

bool run_prover_rete_coq_single(rete_net_state* state){
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
	state->end_of_branch = next;
	check_state_finished(state);
	write_proof_node(state, next);

	//delete_rete_state(state, state->parent);
	//destroy_substitution_memory(& state->local_subst_mem);

	return true;
      } else {	// not goal rule
	if(next->rule->rhs->n_args > 1){
	  write_proof_node(state, next);
	  if(state->net->treat_all_disjuncts){
	    insert_rete_disjunction_coq_single(state, next, get_current_state_step_no(state));
	    return true;
	  } else { // !state->net->treat_all_disjuncts
	    bool rv = start_rete_disjunction_coq_single(state, next);
	    return rv;
	  }
	} else { // rhs is single conjunction
	  insert_rete_net_conjunction(state, next->rule->rhs->args[0], next->substitution);
	  write_proof_node(state, next);
	  write_proof_edge(state, state);  

	}
      }
    } // end while(true)
 }

/**
   Auxiliary function called from (insert|start)_rete_net_disjunction_coq_single
**/
rete_net_state* run_rete_proof_disj_branch(rete_net_state* state, conjunction* con, substitution* sub, unsigned int branch_no){
  rete_net_state* copy_state = split_rete_state(state, branch_no);
  write_proof_edge(state, copy_state);
  insert_rete_net_conjunction(copy_state, con, sub);
  return copy_state;
}

/**
   Called from the thread runner when a disjunction is popped which is treated for the second time.

   The disj_ri_stack_mutex MUST NOT be locked when this function is called.

   If net->treat_all_disjuncts is false, the first branch has already been run by start_rete_disjunction_coq_single below.
**/
void insert_rete_disjunction_coq_single(rete_net_state* state, rule_instance* next, unsigned int step){
  unsigned int i;
  assert(state->net->treat_all_disjuncts || state->branches[0]->finished);

  if(!state->net->treat_all_disjuncts && !next->used_in_proof){
    rete_net_state* copy_state = state->branches[0];
    transfer_state_endpoint(state, copy_state);
    for(i = 0; i < state->end_of_branch->rule->rhs->n_args; i++){
      assert(state != state->branches[i] && copy_state != state->branches[i]);
    }
    assert(get_current_state_step_no(state) > step);
    check_state_finished(state);
    return;
  }
  
  if(state->net->treat_all_disjuncts){
    state->end_of_branch = next;
    state->branches = calloc_tester(next->rule->rhs->n_args, sizeof(rete_net_state*));
    i = 0;
  } else 
    i = 1;
  
  for(; i < next->rule->rhs->n_args; i++){
    state->branches[i] = run_rete_proof_disj_branch(state, next->rule->rhs->args[i], next->substitution, i);
#ifdef HAVE_PTHREAD
    pthread_mutex_lock(disj_ri_stack_mutex);
#endif
    push_ri_state_stack(disj_ri_stack, NULL, state->branches[i], step);
#ifdef HAVE_PTHREAD
    pthread_cond_signal(& stack_increased_cond);
    pthread_mutex_unlock(disj_ri_stack_mutex);
#endif
  }
  for(i = 0; i < next->rule->rhs->n_args; i++)
    assert(state != state->branches[i]);
  return;
}

/**
   Finds the newest element on disj_ri_stack with a finished branch
   The function assumes the coresponding mutex is already locked

   Also assumes that it is called with a non-empty disj_ri_stack
**/
bool pop_latest_finished_disj(rule_instance ** next, rete_net_state ** state, unsigned int * step){
  rule_instance * next_tmp;
  rete_net_state * state_tmp;
  unsigned int  step_tmp;
  bool retval;

  pop_ri_state_stack(disj_ri_stack, &next_tmp, &state_tmp, &step_tmp);
  *next = next_tmp;
  *state = state_tmp;
  *step = step_tmp;

  if(next_tmp == NULL || (*state)->net->treat_all_disjuncts || (*state)->branches[0]->finished){
    return true;
  } else {
    if(is_empty_ri_state_stack(disj_ri_stack))
      retval = false;
    else 
      retval = pop_latest_finished_disj(next, state, step);
    push_ri_state_stack(disj_ri_stack, next_tmp, state_tmp, step_tmp);
    return retval;
  }
}

bool thread_runner_returns(bool retval){
#ifdef HAVE_PTHREAD
  pthread_mutex_unlock(disj_ri_stack_mutex);
#endif
  return retval;
}

bool thread_runner_single_step(void){
  rete_net_state* state = NULL; 
  rule_instance* next;
  unsigned int step;
  bool rp_val;
  bool has_popped = false;
  unsigned int i, retcode;

#ifdef HAVE_PTHREAD
  pthread_mutex_lock(disj_ri_stack_mutex);
#endif
  
  while(!has_popped){
    if(!foundproof) 
      return thread_runner_returns(false);
    if(is_empty_ri_state_stack(disj_ri_stack)){
      if(provers_running == 0)
	return thread_runner_returns(true);
    } else {
      if( pop_latest_finished_disj(&next, &state, &step) ){
	has_popped = true;
	break;
      }
    }
#ifdef HAVE_PTHREAD
    retcode = pthread_cond_wait(&stack_increased_cond,  disj_ri_stack_mutex);
    if(retcode != 0)
      perror("prover.c: thread_runner_single_step: could not wait on on signal ");
#endif
  }
  assert(has_popped && foundproof && state != NULL);
  assert(next == NULL || state->net->treat_all_disjuncts || state->branches[0]->finished);

  provers_running++;
#ifdef HAVE_PTHREAD
  pthread_mutex_unlock(disj_ri_stack_mutex);
#endif
  
  if(next == NULL){
    rp_val = run_prover_rete_coq_single(state);
  } else {
    assert(!state->net->treat_all_disjuncts && state->branches[0]->finished);
    rp_val = true;
    insert_rete_disjunction_coq_single(state, next, step);
  }

#ifdef HAVE_PTHREAD
  pthread_mutex_lock(disj_ri_stack_mutex);
#endif
  provers_running--;
#ifdef HAVE_PTHREAD
  pthread_cond_broadcast(& prover_done_cond);
  pthread_cond_broadcast(& stack_increased_cond);
  pthread_mutex_unlock(disj_ri_stack_mutex);
#endif
  return rp_val;
}

/**
   This runs the single-threaded prover

   One instance of this function is run in each thread
**/
void * thread_runner(void * arg){
  int tid;
  if(arg == NULL)
    tid = 0;
  else
    tid = * (int *) arg;
#ifdef HAVE_PTHREAD
  pthread_mutex_lock(disj_ri_stack_mutex);
#endif
  while(foundproof && (!is_empty_ri_state_stack(disj_ri_stack) || provers_running > 0)){
#ifdef HAVE_PTHREAD
    pthread_mutex_unlock(disj_ri_stack_mutex);
#endif
    if(!thread_runner_single_step())
      break;
  }
#ifdef HAVE_PTHREAD
  pthread_cond_broadcast(& prover_done_cond);
  pthread_cond_broadcast(& stack_increased_cond);
  pthread_mutex_unlock(disj_ri_stack_mutex);
#endif
  return NULL;
}

#ifdef HAVE_PTHREAD
/**
   Starts the threads, and wait till some signal it is done
**/
void thread_manager(int n_threads){
  pthread_t * tids;
  unsigned int i;
  int * tid_infos;
  tid_infos = calloc(n_threads, sizeof(int));
  tids = calloc(n_threads, sizeof(pthread_t));
  for(i = 0; i < n_threads; i++){
    tid_infos[i] = i;
    pthread_create(& tids[i], NULL, thread_runner, (void *) &tid_infos[i]);
  }
  pthread_mutex_lock(disj_ri_stack_mutex);
  while(foundproof && (!is_empty_ri_state_stack(disj_ri_stack) || provers_running > 0))
    pthread_cond_wait(&prover_done_cond,  disj_ri_stack_mutex);

  assert( !foundproof || (provers_running == 0 && is_empty_ri_state_stack(disj_ri_stack)));

  if(pthread_cond_broadcast(& stack_increased_cond) != 0)
    perror("prover.c:thread_manager, could not broadcast signal");
  pthread_mutex_unlock(disj_ri_stack_mutex);
  
  
  for(i = 0; i < n_threads; i++){
    int err_no = pthread_join(tids[i], NULL);
    if(err_no != 0)
      perror("prover.c : thread_manager : Could not join thread");
  }
}
#endif

/**
   Called from run_prover_rete_coq_single when meeting a disjunction and net->treat_all_disjuncts is false
   Runs the first branch. 
   The remaining branches are then treated later, in 
   insert_rete_disjunction_coq_single, when we know if the disjunction 
   needs to  be treated.

   Puts the disjunction on the stack. 

**/
bool start_rete_disjunction_coq_single(rete_net_state* state, rule_instance* next){
  const axiom* rule;
  unsigned int i;
  unsigned int step;
  bool retval;
  rete_net_state* copy_state;
  bool rp_val;

  rule = next->rule;

  assert(!state->net->treat_all_disjuncts && rule->rhs->n_args > 1);

  state->end_of_branch = next;

  state->branches = calloc_tester(rule->rhs->n_args, sizeof(rete_net_state*));
  step = get_current_state_step_no(state);

  state->branches[0] = run_rete_proof_disj_branch(state, rule->rhs->args[0], next->substitution, 0);
  
#ifdef HAVE_PTHREAD
  pthread_mutex_lock(disj_ri_stack_mutex);
#endif
  push_ri_state_stack(disj_ri_stack, next, state, step);
  
#ifdef HAVE_PTHREAD
  pthread_cond_signal(& stack_increased_cond);
  pthread_mutex_unlock(disj_ri_stack_mutex);
#endif

  rp_val  = run_prover_rete_coq_single(state->branches[0]);
  if(!rp_val)
    return false;

  return true;
}




/**
   Writes the coq proof after the prover is done.
   Used by the multithreaded version
**/
void write_single_coq_proof(rete_net_state* state){
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
      write_single_coq_proof(state->branches[i]);
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
/**
   The main prover function

   Creates and deletes a RETE state on the heap
   
   If factset is true, it does not use a rete network

   Returns 0 if no proof found, otherwise, the number of steps
**/
unsigned int prover(const rete_net* rete, bool multithread){
  unsigned int i, j, retval;
  atom* true_atom;
  bool has_fact;
#ifdef HAVE_PTHREAD
  pthread_mutexattr_t p_attr;
#endif
  rete_net_state* state = create_rete_state(rete, verbose);
  const theory* th = rete->th;

#ifdef HAVE_PTHREAD
  num_threads = 10;
  pthread_mutexattr_init(&p_attr);

  pthread_mutexattr_settype(&p_attr, PTHREAD_MUTEX_ERRORCHECK_NP);

  disj_ri_stack_mutex = malloc_tester(sizeof(pthread_mutex_t));
  history_mutex = malloc_tester(sizeof(pthread_mutex_t));
  
  pthread_mutex_init(disj_ri_stack_mutex, &p_attr);
  pthread_mutex_init(history_mutex, &p_attr);
  pthread_mutexattr_destroy(&p_attr);
#endif

  disj_ri_stack = initialize_ri_state_stack();
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
  
  push_ri_state_stack(disj_ri_stack, NULL, state, get_current_state_step_no(state));
  foundproof = true;
#ifdef HAVE_PTHREAD
  if(multithread){
    thread_manager(num_threads);
    pthread_mutex_destroy(disj_ri_stack_mutex);
    pthread_mutex_destroy(history_mutex);
    free(disj_ri_stack_mutex);
    free(history_mutex);
  } else
#endif
    thread_runner(NULL);

  if(foundproof && rete->coq){
    write_single_coq_proof(state);
    end_proof_coq_writer(rete->th);
  }
  retval = get_latest_global_step_no(state);
  delete_full_rete_state(state);
  delete_ri_state_stack(disj_ri_stack);

  if(foundproof)
    return retval;
  else
    return 0;
}
