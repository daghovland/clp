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
   Auxiliary function for create_rete_state_single
**/
void init_substitution_store_array(substitution_store ** array, unsigned int n_stores, substitution_size_info ssi){
  unsigned int i;
  *array = calloc_tester(n_stores, sizeof(substitution_store));
  for(i = 0; i < n_stores; i++)
    (*array)[i] = init_substitution_store(ssi);
}


/**
   Called after the rete net is created.
   Initializes the substition lists and queues in the state
**/
rete_state_single* create_rete_state_single(const rete_net* net, bool verbose){
  unsigned int i;
  substitution_size_info ssi = net->th->sub_size_info;

  rete_state_single* state = malloc_tester(sizeof(rete_state_single));

  init_substitution_store_array(&state->subs, net->n_subs, net->th->sub_size_info);
  
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


rete_state_backup backup_rete_state(rete_state_single* state){
  rete_state_backup backup;
  unsigned int i;
  backup.sub_backups = backup_substitution_store_array(state->subs, state->net->n_subs);
  if(state->net->has_factset){
    backup.factset_backups = backup_fact_store_array(state->factsets, state->net->th->n_predicates);
    backup.new_facts_backups = calloc(state->net->th->n_predicates, sizeof(fact_store_iter));
    copy_fact_iter_array(backup.new_facts_backups, state->new_facts_iters, state->net->th->n_predicates);
  }
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
  destroy_substitution_store_backup_array(backup->sub_backups, backup->state->net->n_subs);
  if(backup->state->net->has_factset){
    destroy_fact_store_backup_array(backup->factset_backups, backup->state->net->th->n_predicates);
    free(backup->new_facts_backups);
  }
  free(backup->rq_backups);
}

rete_state_single* restore_rete_state(rete_state_backup* backup){
  unsigned int i;
  rete_state_single* state = backup->state;
  for(i = 0; i < state->net->n_subs; i++)
    restore_substitution_store(& state->subs[i], backup->sub_backups[i]);
  if(state->net->has_factset){
    for(i = 0; i < state->net->th->n_predicates; i++)
      restore_fact_store(& state->factsets[i], backup->factset_backups[i]);
    copy_fact_iter_array(state->new_facts_iters, backup->new_facts_backups, state->net->th->n_predicates);
  }
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
  return get_sub_store_iter(& state->subs[node_no]);
}


fact_store_iter get_state_fact_store_iter(rete_state_single* state, unsigned int pred_no){
  return get_fact_store_iter(& state->factsets[pred_no]);
}


/**
   Insert a copy of substition into the state, if not already there (modulo relevant_vars)
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
  assert(test_is_instantiation(rule->rhs->free_vars, sub));
  state->rule_queues[rule->axiom_no] = push_rule_instance_single(state->rule_queues[rule->axiom_no]
								 , rule
								 , sub
								 , get_state_step_no_single(state)
								 , state->net->strat == clpl_strategy
								 );
}

unsigned int axiom_queue_previous_application_single_state(rete_state_single* state, size_t axiom_no){
  return rule_queue_single_previous_application(state->rule_queues[axiom_no]);
}


rule_instance* pop_axiom_rule_queue_single_state(rete_state_single* state, size_t axiom_no){
  return pop_rule_queue_single(& state->rule_queues[axiom_no], get_state_step_no_single(state));
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
      if(remaining_conjunction_true_in_fact_set(state, con, conjunct+1, tmp_sub)){
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
   These two functions both return true if the queue is empty.
   When the net becomes lazy, or multithreaded, the "may_have" version
   will be different, while the has_new will wait till a new one comes out.
**/
bool axiom_has_new_instance_single(rule_queue_state rqs, size_t axiom_no){
  rete_state_single* state = rqs.single;
  while( ! is_empty_axiom_rule_queue_single_state(state, axiom_no)){
    rule_instance* ri = peek_axiom_rule_queue_single_state(state, axiom_no);
    if(!disjunction_true_in_fact_store(state, ri->rule->rhs, & ri->sub))
      return true;
    pop_axiom_rule_queue_single_state(state, axiom_no);
  }
  return false;
}


bool axiom_may_have_new_instance_single_state(rete_state_single* state, size_t axiom_no){
  return ! is_empty_axiom_rule_queue_single_state(state, axiom_no);
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
   Interface to strategy, choose_next_instance and rule_queue_state
**/

bool axiom_may_have_new_instance_single(rule_queue_state rqs, size_t axiom_no){
  return ! is_empty_axiom_rule_queue_single_state(rqs.single, axiom_no);
}


bool is_empty_axiom_rule_queue_single(rule_queue_state rqs, size_t axiom_no){
  return  is_empty_axiom_rule_queue_single_state(rqs.single, axiom_no);
}


unsigned int axiom_queue_previous_application_single(rule_queue_state rqs, size_t axiom_no){
  return axiom_queue_previous_application_single_state(rqs.single, axiom_no);
}


rule_instance* pop_axiom_rule_queue_single(rule_queue_state rqs, size_t axiom_no){
  return pop_axiom_rule_queue_single_state(rqs.single, axiom_no);
}


unsigned int rule_queue_possible_age_single(rule_queue_state rqs, size_t axiom_no){
  return rule_queue_single_age(rqs.single->rule_queues[axiom_no]);
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
