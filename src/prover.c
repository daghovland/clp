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
#include "proof_writer.h"
#include "substitution.h"
#include "substitution_store_mt.h"
#include "rule_instance_state_stack.h"
#include "instantiate.h"
#include "rete.h"
#include "rete_state.h"
#include "error_handling.h"
#ifdef HAVE_PTHREAD
#include <pthread.h>
#endif
#include <errno.h>

extern bool debug, verbose;

bool foundproof;

unsigned int provers_running;

#ifdef HAVE_PTHREAD
unsigned int num_threads;
#define MAX_THREADS 20

pthread_cond_t stack_increased_cond = PTHREAD_COND_INITIALIZER;
pthread_cond_t prover_done_cond = PTHREAD_COND_INITIALIZER;

pthread_mutex_t * history_mutex;// = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t * disj_ri_stack_mutex;// = PTHREAD_MUTEX_INITIALIZER;
#endif

/*
  A stack of used, but not proved disjunctive rules

  Pushed in "check_used_rule_instance" and popped in run_prover

  initialized by prover()
**/
rule_instance_state_stack* disj_ri_stack;

rule_instance_state * * history;
size_t size_history;

bool insert_rete_net_disjunction(rete_net_state*, rule_instance*, bool);
bool start_rete_disjunction_coq_mt(rete_net_state* state, rule_instance* next);
void insert_rete_disjunction_coq_mt(rete_net_state* state, rule_instance* next, timestamp step);
bool thread_runner_single_step(void);
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
   Wrappers for write_proof_node
**/
void print_state_rule_queues_rqs(rule_queue_state rqs, FILE* f){
  print_state_rule_queues(rqs.state, f);
}


void print_state_new_facts_rqs(rule_queue_state rqs, FILE* f){
  print_state_new_facts(rqs.state, f);
}

void write_prover_node_old(rete_net_state* state, rule_instance* next){
  rule_queue_state rqs;
  rqs.state = state;
  write_proof_node(state->step_no, get_current_state_step_no(state), state->proof_branch_id, state->net, rqs, state->constants, print_state_new_facts_rqs, print_state_rule_queues_rqs, next);
}

/**
   Called at the end of a disjunctive branch
   Checks what rule instances were used
   Called from run_prover_rete_coq_mt

   At the moment, all rule instances are unique in eqch branch (they are copied when popped from the queue)
   So we know that changing the "used_in_proof" here is ok.
**/
void check_used_rule_instances_coq_mt(rule_instance* ri, rete_net_state* historic_state, timestamp historic_ts, timestamp current_ts){
  substitution_size_info ssi = historic_state->net->th->sub_size_info;
  assert(ri == history[historic_ts.step]->ri);
  if(!ri->used_in_proof && (ri->rule->type != fact || ri->rule->rhs->n_args > 1)){
    timestamps_iter iter = get_sub_timestamps_iter(& ri->sub);
    if(ri->rule->rhs->n_args <= 1){
      ri = copy_rule_instance(ri, ssi);
      history[historic_ts.step]->ri = ri;
    }
    ri->used_in_proof = true;
    while(has_next_timestamps_iter(&iter)){
      timestamp premiss_no = get_next_timestamps_iter(&iter);
      if(!is_init_timestamp(premiss_no)){
	assert(premiss_no.step == history[premiss_no.step]->step_no.step);
	check_used_rule_instances_coq_mt((history[premiss_no.step])->ri, (history[premiss_no.step])->s, (history[premiss_no.step])->step_no, current_ts);
      }
    }
    destroy_timestamps_iter(&iter);
    if(ri->rule->rhs->n_args == 1 && ri->rule->rhs->args[0]->is_existential)
      push_ri_stack(historic_state->elim_stack, historic_ts, historic_ts, current_ts);
  }
}


/**
   Create rule instance history. Called from run_prover
   Used by check_used_rule_instances and coq proof output
**/
void insert_rule_instance_history(rete_net_state* s, rule_instance* ri){
  unsigned int step = get_current_state_step_no(s);
#ifdef HAVE_PTHREAD
  if(step >= size_history -  1){
    pt_err(pthread_mutex_lock(history_mutex), "Could not get lock on history array.\n");
#endif
    while(step >= size_history - 1){
#ifndef NDEBUG
      unsigned int i;
#endif
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
  history[step] = create_rule_instance_state(ri, s, create_normal_timestamp(get_current_state_step_no(s)));
}

void insert_rete_net_conjunction(rete_net_state* state, 
				 conjunction* con, 
				 substitution* sub){
  unsigned int i;
  assert(test_conjunction(con));
  assert(test_substitution(sub));

  // At the moment the substitution is copied, when the instance is popped, 
  // so we know it is unique. This could be change if memory becomes an issue.
  if(!state->net->existdom)
    fresh_exist_constants(con, sub, state->constants);
  
  assert(test_is_conj_instantiation(con, sub));
  
  for(i = 0; i < con->n_args; i++){
    atom* ground = instantiate_atom(con->args[i], sub);
    
    assert(test_ground_atom(ground));
    
    
#ifdef __DEBUG_RETE_STATE
    printf("New fact: ");
    print_fol_atom(ground, state->constants, stdout);
    printf("\n");
#endif
    
    insert_state_fact_set(state->factset, ground, get_current_state_step_no(state), state->constants);
    if(!state->net->factset_lhs || state->net->use_beta_not)
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
  print_state_fact_set(state->factset, state->constants, stdout, state->net->th->n_predicates);
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
   Called when reaching goal state in run_prover_rete_coq_mt
   The finished is read from insert_rete_disjunction_coq_mt
**/
void check_state_finished(rete_net_state* state){
  rete_net_state* parent = (rete_net_state*) state->parent;
  unsigned int n_branches;
#ifndef NDEBUG
  bool found_self = false;
#endif
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
#ifndef NDEBUG
      found_self = true;
#endif
    }
  }
  assert(found_self);
  check_state_finished(parent);
}

/**
   Runs a branch in the proof.

   Should be thread-safe. 

   Note that the rule instance on the stack is discarded, as the 
   conjunction is already inserted by insert_rete_net_disjunction_coq_mt below
**/

bool run_prover_rete_coq_mt(rete_net_state* state){
    while(true){
      unsigned int ts;
      rule_instance* next = choose_next_instance_state(state);
      if(next == NULL)
	return return_found_model(state);
      bool incval = inc_proof_step_counter(state);
      if(!incval)
	return return_reached_max_steps(state, next);
      insert_rule_instance_history(state, next);
      ts = get_current_state_step_no(state);
      if(next->rule->type == goal || next->rule->rhs->n_args == 0){
	timestamp stamp = create_normal_timestamp(ts);
	check_used_rule_instances_coq_mt(next, state, stamp, stamp);
	state->end_of_branch = next;
	check_state_finished(state);
	write_prover_node_old(state, next);

	//delete_rete_state(state, state->parent);
	//destroy_substitution_store_mt(& state->local_subst_mem);

	return true;
      } else {	// not goal rule
	if(next->rule->rhs->n_args > 1){
	  write_prover_node_old(state, next);
	  if(state->net->treat_all_disjuncts){
	    timestamp stamp = create_normal_timestamp(get_current_state_step_no(state));
	    insert_rete_disjunction_coq_mt(state, next, stamp);
	    return true;
	  } else { // !state->net->treat_all_disjuncts
	    bool rv = start_rete_disjunction_coq_mt(state, next);
	    return rv;
	  }
	} else { // rhs is single conjunction
	  insert_rete_net_conjunction(state, next->rule->rhs->args[0], & next->sub);
	  write_prover_node_old(state, next);
	  write_proof_edge(state->proof_branch_id, state->step_no, state->proof_branch_id, state->step_no);  

	}
      }
    } // end while(true)
 }

/**
   Auxiliary function called from (insert|start)_rete_net_disjunction_coq_mt
**/
rete_net_state* run_rete_proof_disj_branch(rete_net_state* state, conjunction* con, substitution* sub, unsigned int branch_no){
  rete_net_state* copy_state = split_rete_state(state, branch_no);
  write_proof_edge(state->proof_branch_id, state->step_no, copy_state->proof_branch_id, copy_state->step_no);
  insert_rete_net_conjunction(copy_state, con, sub);
  return copy_state;
}

/**
   Called from the thread runner when a disjunction is popped which is treated for the second time.

   The disj_ri_stack_mutex MUST NOT be locked when this function is called.

   If net->treat_all_disjuncts is false, the first branch has already been run by start_rete_disjunction_coq_mt below.
**/
void insert_rete_disjunction_coq_mt(rete_net_state* state, rule_instance* next, timestamp step){
  unsigned int i;
  assert(state->net->treat_all_disjuncts || state->branches[0]->finished);

  if(!state->net->treat_all_disjuncts && !next->used_in_proof){
    rete_net_state* copy_state = state->branches[0];
    transfer_state_endpoint(state, copy_state);
    for(i = 0; i < state->end_of_branch->rule->rhs->n_args; i++){
      assert(state != state->branches[i] && copy_state != state->branches[i]);
    }
    assert(get_current_state_step_no(state) > step.step);
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
    state->branches[i] = run_rete_proof_disj_branch(state, next->rule->rhs->args[i], & next->sub, i);
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
bool pop_latest_finished_disj(rule_instance ** next, rete_net_state ** state, timestamp * step){
  rule_instance * next_tmp;
  rete_net_state * state_tmp;
  timestamp  step_tmp;
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
  timestamp step;
  bool rp_val;
  bool has_popped = false;
  unsigned int retcode;

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
    rp_val = run_prover_rete_coq_mt(state);
  } else {
    assert(!state->net->treat_all_disjuncts && state->branches[0]->finished);
    rp_val = true;
    insert_rete_disjunction_coq_mt(state, next, step);
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
   Called from run_prover_rete_coq_mt when meeting a disjunction and net->treat_all_disjuncts is false
   Runs the first branch. 
   The remaining branches are then treated later, in 
   insert_rete_disjunction_coq_mt, when we know if the disjunction 
   needs to  be treated.

   Puts the disjunction on the stack. 

**/
bool start_rete_disjunction_coq_mt(rete_net_state* state, rule_instance* next){
  const axiom* rule;
  timestamp step;
  bool rp_val;

  rule = next->rule;

  assert(!state->net->treat_all_disjuncts && rule->rhs->n_args > 1);

  state->end_of_branch = next;

  state->branches = calloc_tester(rule->rhs->n_args, sizeof(rete_net_state*));
  step = create_normal_timestamp(get_current_state_step_no(state));

  state->branches[0] = run_rete_proof_disj_branch(state, rule->rhs->args[0], & next->sub, 0);
  
#ifdef HAVE_PTHREAD
  pthread_mutex_lock(disj_ri_stack_mutex);
#endif
  push_ri_state_stack(disj_ri_stack, next, state, step);
  
#ifdef HAVE_PTHREAD
  pthread_cond_signal(& stack_increased_cond);
  pthread_mutex_unlock(disj_ri_stack_mutex);
#endif

  rp_val  = run_prover_rete_coq_mt(state->branches[0]);
  if(!rp_val)
    return false;

  return true;
}


/**
   Instantiates get_history in proof_write
**/
rule_instance* get_history_state(unsigned int no, rule_queue_state rqs){
  return history[no]->ri;
}

/**
   Writes the coq proof after the prover is done.
   Used by the multithreaded version
**/
void write_mt_coq_proof(rete_net_state* state){
  FILE* coq_fp = get_coq_fdes();
  unsigned int n_branches = state->end_of_branch->rule->rhs->n_args;
  timestamp step = create_normal_timestamp(get_current_state_step_no(state));
  timestamp step_ri, pusher;
  rule_queue_state rqs;
  rqs.state = state;
  init_rev_stack(state->elim_stack);
  fprintf(coq_fp, "(* Treating branch %s *)\n", state->proof_branch_id);
  assert(is_empty_ri_stack(state->elim_stack) || ! is_empty_rev_ri_stack(state->elim_stack));
  while(!is_empty_rev_ri_stack(state->elim_stack)){
    rule_instance* ri = history[pop_rev_ri_stack(state->elim_stack, &step_ri, &pusher).step]->ri;
    write_elim_usage_proof(state->net, ri, step_ri, state->constants);
  }
  if(state->end_of_branch->rule->type == goal && n_branches <= 1){
    fprintf(coq_fp, "(* Reached leaf at step %i *)\n", step.step);
    write_goal_proof(state->end_of_branch, state->net, step, get_history_state, rqs, state->constants);
    fprintf(coq_fp, "(* Finished goal proof of leaf at step %i *)\n", step.step);

  } else {
    unsigned int i;
    assert(n_branches > 1);
    write_elim_usage_proof(state->net, state->end_of_branch, step, state->constants);
    for(i = 0; i < n_branches; i++){
      assert(state != state->branches[i]);
      if(i > 0)
	write_disj_proof_start(state->end_of_branch, step, i, state->constants);
      write_mt_coq_proof(state->branches[i]);
    }
    fprintf(coq_fp, "(* Proving lhs of disjunction at %i *)\n", step.step);
    write_premiss_proof(state->end_of_branch, step, state->net, get_history_state, rqs, state->constants);
    fprintf(coq_fp, "(* Finished proof of lhs of step %i *)\n", step.step);
  }
  while(!is_empty_ri_stack(state->elim_stack)){
    rule_instance* ri = history[pop_ri_stack(state->elim_stack, &step_ri, &pusher).step]->ri;
    fprintf(coq_fp, "(* Proving lhs of existential at %i (Used by step %i)*)\n", step_ri.step, pusher.step);
    write_premiss_proof(ri, step_ri, state->net, get_history_state, rqs, state->constants);
    fprintf(coq_fp, "(* Finished proving lhs of existential at %i *)\n", step_ri.step);
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
  unsigned int i, retval;
  atom* true_atom;
  bool has_fact;
#ifdef HAVE_PTHREAD
  pthread_mutexattr_t p_attr;
#endif
  rete_net_state* state = create_rete_state(rete, verbose);

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
  for(i = 0; i < rete->th->n_init_model; i++){
    has_fact = true;
    substitution* sub = create_empty_substitution(rete->th, state->global_subst_mem);
    insert_rete_net_conjunction(state, rete->th->init_model[i], sub);
  }
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
  insert_state_fact_set(state->factset, true_atom, get_current_state_step_no(state), state->constants);

  if(!state->net->factset_lhs && state->net->use_beta_not)
    insert_rete_net_fact(state, true_atom);
  
  push_ri_state_stack(disj_ri_stack, NULL, state, create_normal_timestamp(get_current_state_step_no(state)));
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
    write_mt_coq_proof(state);
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
