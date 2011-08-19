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
#ifdef HAVE_PTHREAD
#include <pthread.h>
#endif
#include <errno.h>

extern bool debug, verbose;

#ifdef HAVE_PTHREAD
unsigned int num_threads;
#define MAX_THREADS 20

pthread_mutex_t prover_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t prover_cond = PTHREAD_COND_INITIALIZER;
#endif


bool insert_rete_net_disj_rule_instance(rete_net_state*, const rule_instance*, bool);

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

bool insert_rete_net_conjunction(rete_net_state* state, 
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
  return true;
}


/**
   The main loop in a single-threaded prover

   Uses a RETE state, but does not allocate or delete it
**/

bool run_prover(rete_net_state* state, bool factset){
  bool empty_queue;
  rule_instance* next;
  while(true){
    /* For factset based search: ( factset 
      &&  ( next = factset_next_instance(state->net->th, state->facts), next != NULL) )*/

    
    if(!factset){
      next = choose_next_instance(state, state->net->strat);
      if(next == NULL){
	break;
      }
    }
    
    assert(test_rule_instance(next, state));
    
    if(next->rule->type != fact || next->rule->rhs->n_args != 1){
      if(!inc_proof_step_counter(state)){
	printf("Reached %i proof steps, higher than given maximum\n", get_global_step_no(state));
	free(next);
	return false;
      }
      
      write_proof_node(state, next);
      
      if(next->rule->type == goal || next->rule->rhs->n_args == 0){
	free(next);
	return true;
      }
    }
    if(state->net->existdom && next->rule->is_existential){
      // This is inspired by Hans de Nivelle's treatment of existential quantifiers.
      // At the moment it is not working and the commandline option -e should probably not be used
      
      unsigned int i, s = 0;
      bool finished = false;
      rete_net_state* copy_state;
      substitution * orig_sub = next->substitution;
      unsigned int n_exist_vars = next->rule->exist_vars->n_vars;
      domain_iter iters[n_exist_vars];
      const term* terms[n_exist_vars];
      bool created_fresh_constant[n_exist_vars];
      
      for(i = 0; i < next->rule->exist_vars->n_vars; i++){
	iters[i] = get_domain_iter(state);
	created_fresh_constant[i] = false;
      }
      
      assert(n_exist_vars > 0);
      
      if(!domain_iter_has_next(state, &iters[0])){
	terms[0] = get_fresh_constant(state,  next->rule->exist_vars->vars[0]);
	created_fresh_constant[0] = true;
      }
      
      for(i = 0; i < next->rule->exist_vars->n_vars; i++)
	terms[i] = domain_iter_get_next(state, & iters[i]);
      
      while(!finished){
	next->substitution = copy_substitution(orig_sub);
	for(i = next->rule->exist_vars->n_vars - 1; i >= 0; i--){
	  if(domain_iter_has_next(state,&iters[i])){
	    terms[i] = domain_iter_get_next(state, &iters[i]);
	    break;
	  } else {
	    if(!created_fresh_constant[i]){
	      terms[i] = get_fresh_constant(state, next->rule->exist_vars->vars[i]);
	      created_fresh_constant[i] = true;
	      break;
	    }
	    if(i == 0){
	      finished = true;
	      break;
	    }
	    iters[i] = get_domain_iter(state);
	    assert(domain_iter_has_next(state, &iters[i]));
	    terms[i] = domain_iter_get_next(state, &iters[i]);
	  } // end not iterator has next element
	} // end for iterators
	for(i = 0; i < next->rule->exist_vars->n_vars; i++)
	  add_substitution(next->substitution, next->rule->exist_vars->vars[i], terms[i]);
	copy_state = split_rete_state(state, s++);
	write_proof_edge(state, copy_state);  
	if(!insert_rete_net_disj_rule_instance(copy_state, next, factset)){
	  free(next);
	  return false;
	}
	delete_rete_state(copy_state, state);
      }// end while(!finished)
      free(next);
      return true;
    } else {
      // if not existential rule or not existdom. This is usually used
      if(next->rule->rhs->n_args > 1){
	bool retval = insert_rete_net_disj_rule_instance(state, next, factset);
	// free(next);
	return retval;
      } else {
	if(next->rule->type != fact)
	  write_proof_edge(state, state);
	insert_rete_net_conjunction(state, next->rule->rhs->args[0], next->substitution, factset);
      }
    } // end if existential rule
  } // end while queue not empty
  fprintf(stdout, "Found a model of the theory: \n");
  print_fact_set(state->facts, stdout);
  free(next);
  return false;
}  

/**
   The main loop in a multi-threaded prover

   Uses a RETE state, but does not allocate or delete it
**/
#ifdef HAVE_PTHREAD 
bool run_prover(rete_net_state* state){
  while(state->rule_queue->n_queue != 0){
    rule_instance* next;
#ifdef __DEBUG_RETE_PTHREAD
    fprintf(stdout, "Started iteration in thread %li \n", (long) pthread_self());
    print_rete_state(state, stdout);
#endif
    next = choose_next_instance(state);

    assert(test_rule_instance(next, state));
    
    if(!inc_proof_step_counter(state)){
      printf("Reached %i proof steps, higher than given maximum\n", get_global_step_no(state));
      return false;
    }

    write_proof_node(state, next);
    
    if(verbose){
      fprintf(stdout, "Applying rule instance: ");
      print_rule_instance(next, stdout);
      fprintf(stdout, "\n");
    }
    if(next->rule->type == goal || next->rule->rhs->n_args == 0){
#ifdef __DEBUG_RETE_PTHREAD
      printf("Exiting run_prover in thread %li\n",  (long) pthread_self());
#endif
      return true;
    }
    if(next->rule->rhs->n_args > 1){
      bool retval = insert_rete_net_disj_rule_instance(state,next);
      return retval;
    } else {
      write_proof_edge(state, state);
      insert_rete_net_conjunction(state, next->rule->rhs->args[0], next->substitution);
    }
  } // end while queue not empty
  fprintf(stdout, "Found a model of the theory: \n");
  print_fact_set(state->facts, stdout);
  return false;
}  
#endif

#ifdef HAVE_PTHREAD
static void* run_prover_thread(void* arg){
  bool* retval = malloc_tester(sizeof(bool));
  rete_net_state* state = (rete_net_state*) arg;
#ifdef __DEBUG_RETE_PTHREAD
  printf("Entering thread %li\n", pthread_self());
#endif
  *retval =  run_prover(state);
#ifdef __DEBUG_RETE_PTHREAD
  printf("Exiting thread %li\n", pthread_self());
#endif
  return retval;
}
#endif

/**
   Inserting a disjunctive rule into a network

   Creates and deletes new states for each thread

   Note that goal rules should never be inserted (This does not make sense, as no facts would be inserted)

  **/
#ifdef HAVE_PTHREAD
bool mt_insert_rete_net_disj_rule_instance(rete_net_state* state, const rule_instance* ri){
  unsigned int i, j;
  const disjunction* dis = ri->rule->rhs;
  bool thread_can_run;

  assert(test_rule_instance(ri, state));
  assert(dis->n_args > 1);

  rete_net_state* copy_states[dis->n_args];

  pthread_t tids[dis->n_args]; 
  thread_can_run = num_threads < MAX_THREADS;

  while(!thread_can_run){
    pthread_error_test( pthread_mutex_lock(&prover_mutex), " insert_rete_net_disj_rule_instance: Could not lock prover_mutex");
    pthread_error_test( pthread_cond_wait(&prover_cond, &prover_mutex), " insert_rete_net_disj_rule_instance: Could not lock prover_mutex");
    if( num_threads < MAX_THREADS ){
      num_threads += dis->n_args;
      thread_can_run = true;
    }
    pthread_error_test( pthread_mutex_unlock(&prover_mutex), " insert_rete_net_disj_rule_instance: Could not lock prover_mutex");
  }

  for(i = 0; i < dis->n_args; i++){
    copy_states[i] = split_rete_state(state, i);
    write_proof_edge(state, copy_states[i]);
    insert_rete_net_conjunction(copy_states[i], dis->args[i], ri->substitution);

    pthread_error_test( pthread_create(&tids[i], NULL, &run_prover_thread, copy_states[i]) , "Could not create thread");

  }
  for(i=0; i < dis->n_args; i++){
    void* retstate = malloc_tester(sizeof(bool));
    bool  retval;
#ifdef __DEBUG_RETE_PTHREAD
    printf("Waiting for thread %li\n", (long) tids[i]);
#endif
    
    pthread_error_test( pthread_join(tids[i], &retstate), "Could not wait for thread");
    
    num_threads--;
    pthread_error_test( pthread_cond_signal( &prover_cond), "Could not signal prover condititon\n");
    
    retval = * (bool*) retstate;
    free(retstate);
    
#ifdef __DEBUG_RETE_PTHREAD
    printf("Got return from thread %i\n", tids[i]);
#endif
    
    delete_rete_state(copy_states[i]);
    if(!retval){
      for(; i < dis->n_args; i++){
	pthread_join(tids[i], &retstate);
      }
      return false;
    }
  }
  return true;
}
#endif
/**
   Inserting a disjunctive rule into a network single threaded

   Creates and deletes new states for each thread

   Note that goal rules should never be inserted (This does not make sense, as no facts would be inserted)

  **/
bool insert_rete_net_disj_rule_instance(rete_net_state* state, const rule_instance* ri, bool factset){
  unsigned int i, j;
  const disjunction* dis = ri->rule->rhs;
  bool thread_can_run;

  assert(test_rule_instance(ri, state));
  assert(dis->n_args > 0);

  rete_net_state* copy_states[dis->n_args];
  
  for(i = 0; i < dis->n_args; i++){
    copy_states[i] = split_rete_state(state, i);
    write_proof_edge(state, copy_states[i]);
    insert_rete_net_conjunction(copy_states[i], dis->args[i], ri->substitution, factset);
    
    if(!run_prover(copy_states[i], factset)){
      delete_rete_state(copy_states[i], state);
      return false;
    }
    delete_rete_state(copy_states[i], state);
  }
  //write_proof_edge(state, state);
  //insert_rete_net_conjunction(state, dis->args[dis->n_args - 1], ri->substitution);
  return true;
  //return run_prover(state);
}
/**
   The main prover function

   Creates and deletes a RETE state on the heap
   
   If factset is true, it does not use a rete network

   Returns 0 if no proof found, otherwise, the number of steps
**/
unsigned int prover(const rete_net* rete, bool factset){
  unsigned int i, j, retval;
  bool foundproof;
  rete_net_state* state = create_rete_state(rete, verbose);
  const theory* th = rete->th;
#ifdef HAVE_PTHREAD
  num_threads = 0;
#endif
  srand(1000);
  for(i=0; i < th->n_axioms; i++){
    if(th->axioms[i]->type == fact && th->axioms[i]->rhs->n_args == 1){
      assert(th->axioms[i]->axiom_no == i);
      //      if(factset)
	insert_rete_net_conjunction(state, th->axioms[i]->rhs->args[0], create_substitution(th, 1), factset);
	//      else
	//add_rule_to_queue(th->axioms[i],  create_substitution(th,1), state);
    }
  }
  foundproof =  run_prover(state, factset);
  retval = get_global_step_no(state);
  delete_full_rete_state(state);
  if(foundproof)
    return retval;
  else
    return 0;
}
