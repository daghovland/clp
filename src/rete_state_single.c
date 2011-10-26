/* rete_state_single.c

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
#include "substitution_store_array.h"
#include "rule_queue_single.h"
#include "rete_state_single.h"
#include "rete_worker_queue.h"
#include "rete_worker.h"
#include "substitution.h"
#include "theory.h"
#include "rete.h"
#include <string.h>

/**
   Called after the rete net is created.
   Initializes the substition lists and queues in the state
**/
rete_state_single* create_rete_state_single(const rete_net* net, bool verbose){
  unsigned int i;
  substitution_size_info ssi = net->th->sub_size_info;
  rete_state_single* state = malloc_tester(sizeof(rete_state_single));
  state->node_subs = init_substitution_store_array(net->th->sub_size_info, net->n_subs);
  state->tmp_subs = init_substitution_store_mt(ssi);
  state->verbose = verbose;
  state->net = net;
  state->fresh = init_fresh_const(net->th->vars->n_vars);
  assert(state->fresh != NULL);
  state->constants = init_constants(net->th->vars->n_vars);
  state->step = 0;
  state->rule_queues = calloc_tester(net->th->n_axioms, sizeof(rule_queue_single*));
  state->worker_queues = calloc_tester(net->th->n_axioms, sizeof(rete_worker_queue*));
  state->workers = calloc_tester(net->th->n_axioms, sizeof(rete_worker*));
  for(i = 0; i < net->th->n_axioms; i++){
    state->rule_queues[i] = initialize_queue_single(ssi, i);
    state->worker_queues[i] = init_rete_worker_queue();
    state->workers[i] = init_rete_worker(state->net, & state->tmp_subs, state->node_subs,  state->rule_queues[i], state->worker_queues[i]);
  }
  state->history = initialize_queue_single(ssi, 0);
  if(state->net->has_factset){
    state->factsets = calloc_tester(net->th->n_predicates, sizeof(fact_store));
    state->new_facts_iters = calloc_tester(net->th->n_predicates, sizeof(fact_store_iter));
    for(i = 0; i < net->th->n_predicates; i++){
      state->factsets[i] = init_fact_store();
      state->new_facts_iters[i] = get_fact_store_iter(&state->factsets[i]);
    }
  }
  state->finished = false;
  state->step = 0;
  return state;
}

/**
   Creates information necessary to return to a branching point after treating a branch

   Backs up factsets, rule queues, and node caches. 
**/
rete_state_backup backup_rete_state(rete_state_single* state){
  rete_state_backup backup;
  unsigned int i;
  for(i = 0; i < state->net->th->n_axioms; i++)
    pause_rete_worker(state->workers[i]);
  for(i = 0; i < state->net->th->n_axioms; i++)
    wait_for_worker_to_pause(state->workers[i]);
  backup.node_sub_backups = backup_substitution_store_array(state->node_subs);
  if(state->net->has_factset){
    backup.factset_backups = backup_fact_store_array(state->factsets, state->net->th->n_predicates);
    backup.new_facts_backups = calloc(state->net->th->n_predicates, sizeof(fact_store_iter));
    copy_fact_iter_array(backup.new_facts_backups, state->new_facts_iters, state->net->th->n_predicates);
  }
  backup.rq_backups = calloc_tester(state->net->th->n_axioms, sizeof(rule_queue_single_backup));
  backup.worker_backups = calloc_tester(state->net->th->n_axioms, sizeof(rete_worker_queue_backup));
  for(i = 0; i < state->net->th->n_axioms; i++){
    backup.rq_backups[i] = backup_rule_queue_single(state->rule_queues[i]);
    backup.worker_backups[i] = backup_rete_worker_queue(state->worker_queues[i]);
  }
  backup.state = state;
  for(i = 0; i < state->net->th->n_axioms; i++)
    continue_rete_worker(state->workers[i]);
  return backup;
}


/**
   Returns the current step number of the proving process
**/
unsigned int get_state_step_no_single(rete_state_single* state){
  return state->step;
}

/**
   All substructures of the backup are freed, but not
   the backup itself, since this is assumed to be static in prover_single.c
**/
void destroy_rete_backup(rete_state_backup* backup){
  destroy_substitution_store_array_backup(backup->node_sub_backups, backup->state->net->n_subs);
  if(backup->state->net->has_factset){
    destroy_fact_store_backup_array(backup->factset_backups, backup->state->net->th->n_predicates);
    free(backup->new_facts_backups);
  }
  free(backup->rq_backups);
  free(backup->worker_backups);
}

rete_state_single* restore_rete_state(rete_state_backup* backup){
  unsigned int i;
  rete_state_single* state = backup->state;
  for(i = 0; i < state->net->th->n_axioms; i++)
    stop_rete_worker(state->workers[i]);
  state->node_subs = restore_substitution_store_array(backup->node_sub_backups);
  if(state->net->has_factset){
    for(i = 0; i < state->net->th->n_predicates; i++)
      restore_fact_store(& state->factsets[i], backup->factset_backups[i]);
    copy_fact_iter_array(state->new_facts_iters, backup->new_facts_backups, state->net->th->n_predicates);
  }
  for(i = 0; i < backup->state->net->th->n_axioms; i++){
    backup->state->rule_queues[i] = restore_rule_queue_single(backup->state->rule_queues[i], & backup->rq_backups[i]);
    backup->state->worker_queues[i] = restore_rete_worker_queue(backup->state->worker_queues[i], & backup->worker_backups[i]);
  }
  for(i = 0; i < state->net->th->n_axioms; i++)
    restart_rete_worker(state->workers[i]);
  return backup->state;
}


/**
   Completely deletes rete state. Called at the end of prover() in prover.c
**/
void delete_rete_state_single(rete_state_single* state){
  unsigned int i;
  for(i = 0; i < state->net->th->n_axioms; i++)
    destroy_rete_worker(state->workers[i]);
  destroy_substitution_store_array(state->node_subs);
  if(state->net->has_factset){
    for(i = 0; i < state->net->th->n_predicates; i++){
      destroy_fact_store(& state->factsets[i]);
      destroy_fact_store_iter(& state->new_facts_iters[i]);
    }
    free(state->factsets);
    free(state->new_facts_iters);
  }
  for(i = 0; i < state->net->th->n_axioms; i++){
    destroy_rule_queue_single(state->rule_queues[i]);
  }
  destroy_substitution_store_mt(& state->tmp_subs);
  free(state->rule_queues);
  destroy_constants(& state->constants);

  
  free(state);
}
  
unsigned int get_state_step_single(const rete_state_single* state){
  return state->step;
}


bool inc_proof_step_counter_single(rete_state_single* state){
  state->step ++;
  return(state->net->maxsteps == 0 || state->step < state->net->maxsteps);
}

sub_store_iter get_state_sub_store_iter(rete_state_single* state, unsigned int node_no){
  return get_array_sub_store_iter(state->node_subs, node_no);
}


fact_store_iter get_state_fact_store_iter(rete_state_single* state, unsigned int pred_no){
  return get_fact_store_iter(& state->factsets[pred_no]);
}


unsigned int axiom_queue_previous_application_single_state(rete_state_single* state, size_t axiom_no){
  return rule_queue_single_previous_application(state->rule_queues[axiom_no]);
}

/**
   This function instantiates "pop_axiom" in strategy.c

   Transfers a rule instance from the rule queue to the history. 
   This must be done while the rule queue is locked, since otherwise 
   the worker may push an instance and realloc the queue with the instance in it.
**/
rule_instance* transfer_from_rule_queue_to_history(rete_state_single* state, size_t axiom_no){
  rule_instance *ri, *next;
  rule_queue_single* rq = state->rule_queues[axiom_no];
#ifdef HAVE_PTHREAD
  lock_queue_single(rq);
#endif
  ri = pop_rule_queue_single(rq, get_state_step_no_single(state));
  next = insert_rule_instance_history_single(state, ri);
#ifdef HAVE_PTHREAD
  unlock_queue_single(rq);
#endif
  assert(test_rule_instance(next));
  return next;
}

void add_rule_to_queue_single(const axiom* rule, const substitution* sub, rule_queue_state rqs){
  rete_state_single* state = rqs.single;
  assert(test_is_instantiation(rule->rhs->free_vars, sub));
  push_rule_instance_single((state->rule_queues[rule->axiom_no])
                            , rule
                            , sub
                            , get_state_step_no_single(state)
                            , state->net->strat == clpl_strategy
                            );
}



/**
   Returns true if the corresponding rule queue is empty.
   At the moment this also means there are no pending instances, 
   while in a lazy or multithreaded version, there might be instances waiting 
   to be output by the net.
**/
bool is_empty_axiom_rule_queue_single_state(rete_state_single* state, size_t axiom_no){
  return rule_queue_single_is_empty(state->rule_queues[axiom_no]);
}

rule_instance* peek_axiom_rule_queue_single_state(rete_state_single * state, size_t axiom_no){
  return peek_rule_queue_single(state->rule_queues[axiom_no]);
}


/**
   For testing whether a conjunction with some substitutions already done, is true in the fact set

   Note that sub is changed if it returns true.
**/
bool remaining_conjunction_true_in_fact_store(rete_state_single* state, const conjunction* con, unsigned int conjunct, const substitution* sub){
  fact_store_iter iter;
  unsigned int pred_no;
  substitution* tmp_sub;
  bool found_true = false;

  if(conjunct >= con->n_args)
    return true;

  pred_no = con->args[conjunct]->pred->pred_no;
  iter = get_state_fact_store_iter(state, pred_no);
  tmp_sub = create_empty_substitution(state->net->th, & state->tmp_subs);

  while(!found_true && has_next_fact_store(&iter)){
    const atom* fact = get_next_fact_store(&iter);
    copy_substitution_struct(tmp_sub, sub, state->net->th->sub_size_info);
    if(find_instantiate_sub(con->args[conjunct], fact, tmp_sub)){
      if(remaining_conjunction_true_in_fact_store(state, con, conjunct+1, tmp_sub)){
	found_true = true;
	break;
      }
    }
  }
  free_substitution(tmp_sub);
  return found_true;
}

/**
   Returns true if the conjunction is true in the factset.
   In that case, the given substitution is extended to such an instance.
   sub must have been instantiated and later freed by the calling function.
**/
bool conjunction_true_in_fact_store(rete_state_single* state, const conjunction* con, const substitution* sub){
  return remaining_conjunction_true_in_fact_store(state, con, 0, sub);
}

bool disjunction_true_in_fact_store(rete_state_single* state, const disjunction* dis, const substitution* sub){
  int i;
  for(i = 0; i < dis->n_args; i++){
    if(conjunction_true_in_fact_store(state, dis->args[i], sub))
      return true;
  }
  return false;
}

/**
   Inserts a copy of ri into the history array. 
   Returns a pointer to the position in the array
   Note that this pointer may be invalidated on the next call to this function
   (But not before)
**/
rule_instance* insert_rule_instance_history_single(rete_state_single* state, const rule_instance* ri){
  unsigned int step =  get_state_step_no_single(state) + 1;
  while(get_rule_queue_single_size(state->history) < step) {
    fprintf(stderr, "Pushing dummy rule instance on history for step %i\n", step);
    push_rule_instance_single(state->history, ri->rule, & ri->sub, step, false);
  }
  assert(test_rule_instance(ri));
  return push_rule_instance_single(state->history, ri->rule, & ri->sub, step, false);
}

rule_instance* get_historic_rule_instance(rete_state_single* state, unsigned int step_no){
  return get_rule_instance_single(state->history, step_no);
}

/**
   Called at the end of a disjunctive branch
   Checks what rule instances were used
   Called from run_prover_rete_coq_mt

   At the moment, all rule instances are unique in eqch branch (they are copied when popped from the queue)
   So we know that changing the "used_in_proof" here is ok.
**/
void check_used_rule_instances_coq_single(rule_instance* ri, rete_state_single* state, unsigned int historic_ts, unsigned int current_ts){
  unsigned int i;
  substitution_size_info ssi = state->net->th->sub_size_info;
  if(!ri->used_in_proof && (ri->rule->type != fact || ri->rule->rhs->n_args > 1)){
    timestamps_iter iter = get_sub_timestamps_iter(& ri->sub);
    //    fprintf(stdout, "Setting step %i to used from step %i\n", historic_ts, current_ts);
    ri->used_in_proof = true;
    while(has_next_timestamps_iter(&iter)){
      int premiss_no = get_next_timestamps_iter(&iter);
      if(premiss_no > 0){
	rule_instance* premiss_ri = get_historic_rule_instance(state, premiss_no);
	check_used_rule_instances_coq_single(premiss_ri, state, premiss_no, current_ts);
      }
    }
    destroy_timestamps_iter(&iter);
    //    if(ri->rule->rhs->n_args == 1 && ri->rule->rhs->args[0]->is_existential)
    //      push_ri_stack(historic_state->elim_stack, ri, historic_ts, current_ts);
  }
}


bool axiom_may_have_new_instance_single_state(rete_state_single* state, size_t axiom_no){
  return ! is_empty_axiom_rule_queue_single_state(state, axiom_no) || ! rete_worker_queue_is_empty(state->worker_queues[axiom_no]) || rete_worker_is_working(state->workers[axiom_no]);
}


/**
   These two functions both return false if the queue is empty.
   When the net becomes lazy, or multithreaded, the "may_have" version
   will be different, while the has_new will wait till a new one comes out.
**/
bool axiom_has_new_instance_single(rule_queue_state rqs, size_t axiom_no){
  rete_state_single* state = rqs.single;
  rule_queue_single* rq = state->rule_queues[axiom_no];
  rete_worker_queue* wq = state->worker_queues[axiom_no];
  bool retval = false;
#ifdef HAVE_PTHREAD
  lock_queue_single(rq);
#endif
  while( axiom_may_have_new_instance_single_state(state, axiom_no)){
    while(rule_queue_single_is_empty(rq) && axiom_may_have_new_instance_single_state(state, axiom_no))
      wait_queue_single(rq);
    if(rule_queue_single_is_empty(rq)){
      retval = false;
      break;
    } else {
      rule_instance* ri = peek_axiom_rule_queue_single_state(state, axiom_no);
#ifdef HAVE_PTHREAD
      unlock_queue_single(rq);
#endif
      if(!disjunction_true_in_fact_store(state, ri->rule->rhs, & ri->sub))
	return true;
#ifdef HAVE_PTHREAD
      lock_queue_single(rq);
#endif
      pop_rule_queue_single(state->rule_queues[axiom_no], get_state_step_no_single(state));
    }
  }
#ifdef HAVE_PTHREAD
  unlock_queue_single(rq);
#endif
  return retval;
}



/**
   Inserts fact into rete network.

   An empty substitution is created on the heap and passed to the network
   This must not be touched after the call to insert_rete_alpha_fact, since it
   deletes or stores it

   Note that calling this function may invalidate any rule_instance pointer, since these all 
   point to members of the rule_queue_single arrays, which may be realloced as a consqequence of this call
**/
void insert_state_rete_net_fact(rete_state_single* state, const atom* fact){
  unsigned int i;
  substitution * tmp_sub = create_empty_substitution(state->net->th, &state->tmp_subs);
  const rete_node* sel = get_const_selector(fact->pred->pred_no, state->net);
  unsigned int step = get_state_step_no_single(state);
  assert(test_atom(fact));
  assert(sel != NULL && sel->val.selector == fact->pred);
  for(i = 0; i < sel->n_children; i++){
    const rete_node* child = sel->children[i];
#ifdef HAVE_PTHREAD
    push_rete_worker_queue(state->worker_queues[child->axiom_no], fact, child, step);
#else
    init_substitution(tmp_sub, state->net->th, step);
    insert_rete_alpha_fact_single(state->net, state->node_caches, &state->tmp_subs, state->rule_queues[child->axiom_no], child, fact, step, tmp_sub);
#endif
  }
  free_substitution(tmp_sub);
}


/**
   Factset functions
**/
void insert_state_factset_single(rete_state_single* state, const atom* ground){
  unsigned int step = get_state_step_no_single(state);
  bool already_in_factset = false;
  unsigned int pred_no = ground->pred->pred_no;
  fact_store_iter iter = get_fact_store_iter(& state->factsets[pred_no]);
  while(has_next_fact_store(&iter)){
    const atom* fact = get_next_fact_store(&iter);
    if(equal_atoms(fact, ground)){
      already_in_factset = true;
      break;
    }
  }
  destroy_fact_store_iter(&iter);
  if(!already_in_factset)
    push_fact_store(& state->factsets[pred_no], ground);
}

/**
   Called when the prover fails, prints the model that satisifes the theory
**/
void print_state_fact_store(rete_state_single* state, FILE* f){
  unsigned int i;
  bool first_iter = true;
  fprintf(f, "{");  
  for(i = 0; i < state->net->th->n_predicates; i++){
    if(first_iter && !is_empty_fact_store(& state->factsets[i]))
      first_iter = false;
    else
      fprintf(f, ", ");
    print_fact_store(& state->factsets[i], f);
  }
  fprintf(f, "}\n");
}

/**
   Called at each step when verbose. Prins the new facts.
   The info is kept in the iterators
**/
void print_state_new_fact_store(rete_state_single* state, FILE* f){
  unsigned int i;
  for(i = 0; i < state->net->th->n_predicates; i++){
    fact_store_iter* iter = & state->new_facts_iters[i];
    while(has_next_fact_store(iter)){
      print_fol_atom(get_next_fact_store(iter), f);
      fprintf(f, ", ");
    }
  }
}

/**
   In the lazy version of rete, we calculuate the maximum age of any instance of a rule by taking either the oldest rule instance already in the queue, 
   or if the queue is empty, the oldest substitution waiting to be inserted. 

   This avoids a bottleneck that occurs when we force at least one instance on the queue when possible.
**/
unsigned int rule_queue_possible_age_single_state(rete_state_single* state, size_t axiom_no){
  rule_queue_single* rq = state->rule_queues[axiom_no];
  rete_worker_queue *wq = state->worker_queues[axiom_no];
  rete_worker *worker = state->workers[axiom_no];
  unsigned int age;
#ifdef HAVE_PTHREAD
  lock_queue_single(rq);
#endif
  if(!rule_queue_single_is_empty(rq)){
    rule_instance * ri = peek_rule_queue_single(rq);
    age = ri->timestamp;
  } else {
#ifdef HAVE_PTHREAD
    lock_worker_queue(wq);
#endif
    if(rete_worker_is_working(worker) || rete_worker_queue_is_empty(wq))
      age = get_worker_step(worker);
    else 
      age = get_timestamp_rete_worker_queue(wq);
#ifdef HAVE_PTHREAD
    unlock_worker_queue(wq);
#endif
  }
#ifdef HAVE_PTHREAD
    unlock_queue_single(rq);
#endif
}


/**
   Called from proof_writer.c via prover.c
**/
void print_state_single_rule_queues(rete_state_single* s, FILE* f){
  unsigned int i;
  for(i = 0; i < s->net->th->n_axioms; i++){
    if(!rule_queue_single_is_empty(s->rule_queues[i])){
      printf("Axiom %s ", s->net->th->axioms[i]->name);
      print_rule_queue_single(s->rule_queues[i], stdout);
    }
  }
}


/**
   Interface to strategy, choose_next_instance and rule_queue_state
**/

bool axiom_may_have_new_instance_single(rule_queue_state rqs, size_t axiom_no){
  return axiom_may_have_new_instance_single_state(rqs.single, axiom_no);
}


bool is_empty_axiom_rule_queue_single(rule_queue_state rqs, size_t axiom_no){
  return  is_empty_axiom_rule_queue_single_state(rqs.single, axiom_no);
}


unsigned int axiom_queue_previous_application_single(rule_queue_state rqs, size_t axiom_no){
  return axiom_queue_previous_application_single_state(rqs.single, axiom_no);
}


rule_instance* pop_axiom_rule_queue_single(rule_queue_state rqs, size_t axiom_no){
  return transfer_from_rule_queue_to_history(rqs.single, axiom_no);
}


unsigned int rule_queue_possible_age_single(rule_queue_state rqs, size_t axiom_no){
  return rule_queue_possible_age_single_state(rqs.single, axiom_no);
}


rule_instance* peek_axiom_rule_queue_single(rule_queue_state rqs, size_t axiom_no){
  return peek_axiom_rule_queue_single_state(rqs.single, axiom_no);
}


rule_instance* choose_next_instance_single(rete_state_single* state)
{
  rule_queue_state rqs;
  rqs.single = state;
  return choose_next_instance(rqs
			      , state->net
			      , state->net->strat
			      , get_state_step_single(state)
			      , is_empty_axiom_rule_queue_single
			      , peek_axiom_rule_queue_single
			      , axiom_has_new_instance_single
			      , rule_queue_possible_age_single
			      , axiom_may_have_new_instance_single
			      , pop_axiom_rule_queue_single
			      , add_rule_to_queue_single
			      , axiom_queue_previous_application_single
			      );
}
