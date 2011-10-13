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
#include "substitution_store.h"
#include "rule_queue_single.h"
#include "rete_state_single.h"
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

  state->subs = calloc_tester(net->n_subs, sizeof(substitution_store));
  for(i = 0; i < net->n_subs; i++)
    state->subs[i] = init_substitution_store(ssi);
  
  state->tmp_subs = init_substitution_store_mt(ssi);
  state->verbose = verbose;
  state->net = net;
  state->fresh = init_fresh_const(net->th->vars->n_vars);
  assert(state->fresh != NULL);
  state->constants = init_constants(net->th->vars->n_vars);
  state->step = 0;

  state->rule_queues = calloc_tester(net->th->n_axioms, sizeof(rule_queue_single*));
  for(i = 0; i < net->th->n_axioms; i++)
    state->rule_queues[i] = initialize_queue_single(ssi);
  
  if(state->net->has_factset){
    state->factset = calloc_tester(sizeof(fact_set*), net->th->n_predicates);
      for(i = 0; i < net->th->n_predicates; i++){
      state->factset[i] = NULL;
      }
  }
  
  state->finished = false;
  state->step = 0;
  return state;
}


rule_instance* choose_next_instance_single(rete_state_single* state)
{
  rule_queue_state rqs;
  rqs.single = state;
  return choose_next_instance(rqs
			      , state->net
			      , state->net->strat
			      , get_state_step_single(state)
			      , state->factset
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

rete_state_backup backup_rete_state(rete_state_single* state){
  rete_state_backup backup;
  unsigned int i;
  backup.sub_backups = calloc_tester(state->net->n_subs, sizeof(substitution_store_backup));
  for(i = 0; i < state->net->n_subs; i++)
    backup.sub_backups[i] = backup_substitution_store(& state->subs[i]);
  backup.rq_backups = calloc_tester(state->net->th->n_axioms, sizeof(rule_queue_single_backup));
  for(i = 0; i < state->net->th->n_axioms; i++)
    backup.rq_backups[i] = backup_rule_queue_single(state->rule_queues[i]);
  backup.state = state;
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
  unsigned int i;
  for(i = 0; i < backup->state->net->n_subs; i++)
    destroy_substitution_backup(& backup->sub_backups[i]);
  free(backup->sub_backups);
  free(backup->rq_backups);
}

rete_state_single* restore_rete_state(rete_state_backup* backup){
  unsigned int i;
  for(i = 0; i < backup->state->net->n_subs; i++)
    restore_substitution_store(& backup->state->subs[i], backup->sub_backups[i]);
  for(i = 0; i < backup->state->net->th->n_axioms; i++)
    backup->state->rule_queues[i] = restore_rule_queue_single(backup->state->rule_queues[i], & backup->rq_backups[i]);
  return backup->state;
}


/**
   Completely deletes rete state. Called at the end of prover() in prover.c
**/
void delete_rete_state_single(rete_state_single* state){
  unsigned int i;
  for(i = 0; i < state->net->n_subs; i++){
    destroy_substitution_store(& state->subs[i]);
  }
  free(state->subs);
  for(i = 0; i < state->net->th->n_axioms; i++){
    destroy_rule_queue_single(state->rule_queues[i]);
  }
  destroy_substitution_store_mt(& state->tmp_subs);
  free(state->rule_queues);
  destroy_constants(& state->constants);

  if(state->net->has_factset){
    for(i = 0; i < state->net->th->n_predicates; i++){
      delete_fact_set(state->factset[i]);
    }
  }

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
  return get_sub_store_iter(& state->subs[node_no]);
}

/**
   Insert a copy of substition into list, if not already there (modulo relevant_vars)
   Used when inserting into alpha- and beta-stores and rule-stores.
   
   Note that this functions is not thread-safe, since substitution_store is not

   The substitution a is not changed or freed. THe calling function must free it after the call returns

**/
bool insert_substitution_single(rete_state_single* state, size_t sub_no, const substitution* a, const freevars* relevant_vars){
  substitution_store store = state->subs[sub_no];
  sub_store_iter iter = get_sub_store_iter(&store);
  
  while(has_next_sub_store(&iter)){
    substitution* next_sub = get_next_sub_store(&iter);
    if(equal_substitutions(a, next_sub, relevant_vars))
      return false;
  }
  destroy_sub_store_iter(&iter);
  push_substitution_sub_store(&store, a);
  return true;
}


void add_rule_to_queue_single(const axiom* rule, const substitution* sub, rule_queue_state rqs){
  rete_state_single* state = rqs.single;
  
  state->rule_queues[rule->axiom_no] = push_rule_instance_single(state->rule_queues[rule->axiom_no]
								 , rule
								 , sub
								 , get_state_step_no_single(state)
								 , state->net->strat == clpl_strategy
								 );
}


unsigned int axiom_queue_previous_application_single(rule_queue_state rqs, size_t axiom_no){
  return rule_queue_single_previous_application(rqs.single->rule_queues[axiom_no]);
}

rule_instance* pop_axiom_rule_queue_single(rule_queue_state rqs, size_t axiom_no){
  rete_state_single* state = rqs.single;
  return pop_rule_queue_single(& state->rule_queues[axiom_no], get_state_step_no_single(state));
}

unsigned int rule_queue_possible_age_single(rule_queue_state rqs, size_t axiom_no){
  return rule_queue_single_age(rqs.single->rule_queues[axiom_no]);
}

/**
   Returns true if the corresponding rule queue is empty.
   At the moment this also means there are no pending instances, 
   while in a lazy or multithreaded version, there might be instances waiting 
   to be output by the net.
**/
bool is_empty_axiom_rule_queue_single(rule_queue_state rqs, size_t axiom_no){
  return rule_queue_single_is_empty(rqs.single->rule_queues[axiom_no]);
}

rule_instance* peek_axiom_rule_queue_single(rule_queue_state rqs, size_t axiom_no){
  return peek_rule_queue_single(rqs.single->rule_queues[axiom_no]);
}

/**
   These two functions both return true if the queue is empty.
   When the net becomes lazy, or multithreaded, the "may_have" version
   will be different, while the has_new will wait till a new one comes out.
**/
bool axiom_has_new_instance_single(rule_queue_state rqs, size_t axiom_no){
  return ! is_empty_axiom_rule_queue_single(rqs, axiom_no);
}


bool axiom_may_have_new_instance_single(rule_queue_state rqs, size_t axiom_no){
  return axiom_has_new_instance_single(rqs, axiom_no);
}


