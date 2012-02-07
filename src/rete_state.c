/* rete_state.c

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

#include <string.h>
#include "common.h"
#include "rete.h"
#include "instantiate.h"

/**
   Wrappers for choose_next_instance in strategy.c
**/
rule_instance* choose_next_instance_state(rete_net_state* state){
  rule_queue_state rqs;
  rqs.state = state;
  return choose_next_instance(rqs
			      , state->net
			      , state->net->strat
			      , get_current_state_step_no(state)
			      , is_empty_axiom_rule_queue_state
			      , peek_axiom_rule_queue_state
			      , axiom_has_new_instance
			      , rule_queue_possible_age
			      , axiom_may_have_new_instance
			      , pop_axiom_rule_queue_state
			      , add_rule_to_queue_state
			      , axiom_queue_previous_application
			      );
}

/**
   Called after the rete net is created.
   Initializes the substition lists and queues in the state
**/
rete_net_state* create_rete_state(const rete_net* net, bool verbose){
  unsigned int i;
  rete_net_state* state = malloc_tester(sizeof(rete_net_state) 
					+ (net->th->n_axioms * sizeof(rule_queue*)));
  state->subs = calloc_tester(sizeof(substitution_list*), net->n_subs);

  state->local_subst_mem = init_substitution_store_mt(net->th->sub_size_info);
  state->global_subst_mem = malloc(sizeof(substitution_store_mt));
  * state->global_subst_mem = init_substitution_store_mt(net->th->sub_size_info);
					  
  state->verbose = verbose;
  for(i = 0; i < net->n_subs; i++){
    state->subs[i] = NULL;
  }
  state->sub_alpha_queues = calloc_tester(sizeof(sub_alpha_queue), net->th->n_axioms);
  for(i = 0; i < net->th->n_axioms; i++)
    state->sub_alpha_queues[i] = init_sub_alpha_queue();
  state->net = net;
  for(i = 0; i < net->th->n_axioms; i++)
    state->axiom_inst_queue[i] = initialize_queue();
  state->step_no = 0;
  state->cursteps = 0;
  state->start_step_no = 0;
  state->global_step_counter = malloc_tester(sizeof(unsigned int));
  * state->global_step_counter = 0;
  state->proof_branch_id = "";
  state->factset = calloc_tester(sizeof(fact_set*), net->th->n_predicates);
  state->prev_factset = calloc_tester(sizeof(fact_set*), net->th->n_predicates);
  for(i = 0; i < net->th->n_predicates; i++){
    state->factset[i] = NULL;
    state->prev_factset[i] = NULL;
  }
  //  state->constants = init_constants(net->th->vars->n_vars);
  state->constants = copy_constants(net->th->constants);
  state->elim_stack = initialize_ri_stack();
  state->finished = false;
  state->parent = NULL;
  return state;
}

/**
   Deletes "state" and much of the linked information.
   Uses "orig", which _must_ be above state in the tree, 
   to find out how much to delete.

   Called during proof
**/
void delete_rete_state(rete_net_state* state, rete_net_state* orig){
  unsigned int i;
  for(i = 0; i < state->net->n_subs; i++){
      delete_substitution_list_below(state->subs[i], orig->subs[i]);
  }
  free(state->subs);
  for(i = 0; i < state->net->th->n_axioms; i++){
    free(state->axiom_inst_queue[i]);
    delete_sub_alpha_queue_below(& state->sub_alpha_queues[i], & orig->sub_alpha_queues[i]);
  }
  free(state->sub_alpha_queues);
  if(strlen(state->proof_branch_id) > 0)
    free((char *) state->proof_branch_id);
  delete_ri_stack(state->elim_stack);

  for(i = 0; i < state->net->th->n_predicates; i++)
    delete_fact_set_below(state->factset[i], orig->factset[i]);

  //  destroy_substitution_store_mt(& state->local_subst_mem);
  destroy_constants(state->constants);
  free(state);
}

/**
   Completely deletes rete state. Called at the end of prover() in prover.c
**/
void delete_full_rete_state(rete_net_state* state){
  unsigned int i;
  for(i = 0; i < state->net->n_subs; i++){
      delete_substitution_list(state->subs[i]);
  }
  free(state->subs);
  for(i = 0; i < state->net->th->n_axioms; i++){
    delete_full_rule_queue(state->axiom_inst_queue[i]);
    delete_sub_alpha_queue_below(& state->sub_alpha_queues[i], NULL);
  }
  free(state->sub_alpha_queues);
  if(strlen(state->proof_branch_id) > 0)
    free((char *) state->proof_branch_id);
  free(state->global_step_counter);
  delete_ri_stack(state->elim_stack);

  for(i = 0; i < state->net->th->n_predicates; i++)
    delete_fact_set(state->factset[i]);
  // destroy_substitution_store_mt(& state->local_subst_mem);
  //destroy_substitution_store_mt(state->global_subst_mem);
  destroy_constants(state->constants);
  free(state);
}

/**
   Copy "constructor"

   Used when treating a disjunctive rule

   size_t is the number of the branch, numbered from left to right, starting at 0
**/
rete_net_state* split_rete_state(rete_net_state* orig, unsigned int branch_no){
  unsigned int i;
  size_t orig_size = sizeof(rete_net_state) + orig->net->th->n_axioms * sizeof(rule_queue*);
  size_t n_subs = orig->net->n_subs;
  size_t n_axioms = orig->net->th->n_axioms;
  size_t branch_id_len;

  rete_net_state* copy = malloc_tester(orig_size);

  memcpy(copy, orig, orig_size);
  copy->subs = calloc_tester(n_subs, sizeof(substitution_list*));
  memcpy(copy->subs, orig->subs, sizeof(substitution_list*) * n_subs);
  copy->local_subst_mem = init_substitution_store_mt(orig->net->th->sub_size_info);
  copy->factset = calloc_tester(orig->net->th->n_predicates, sizeof(fact_set*));
  copy->prev_factset = calloc_tester(orig->net->th->n_predicates, sizeof(fact_set*));
  memcpy(copy->factset, orig->factset, sizeof(fact_set*) * orig->net->th->n_predicates);
  memcpy(copy->prev_factset, orig->factset, sizeof(fact_set*) * orig->net->th->n_predicates);
  
  copy->sub_alpha_queues = calloc_tester(n_axioms, sizeof(sub_alpha_queue*));
  memcpy(copy->sub_alpha_queues, orig->sub_alpha_queues, sizeof(sub_alpha_queue*) * n_axioms);
  for(i = 0; i < n_axioms; i++){
    if(!is_empty_sub_alpha_queue(&copy->sub_alpha_queues[i])){
      copy->sub_alpha_queues[i].end->is_splitting_point = true;
      assert(orig->sub_alpha_queues[i].end->is_splitting_point);
    }
  }

  copy->constants = copy_constants(orig->constants);

  for(i = 0; i < orig->net->th->n_axioms; i++)
    copy->axiom_inst_queue[i] = copy_rule_queue(orig->axiom_inst_queue[i]);
  
  copy->step_no = 0;

  branch_id_len = strlen(orig->proof_branch_id) + 20;
  copy->proof_branch_id = malloc_tester(branch_id_len);
  
  if( snprintf((char *) copy->proof_branch_id, branch_id_len, "%s_%zi", orig->proof_branch_id, branch_no) < 0)
    {
      perror("Could not create new proof branch id\n");
      exit(EXIT_FAILURE);
    }
  assert(strlen(copy->proof_branch_id) < branch_id_len);
  assert(orig->global_step_counter == copy->global_step_counter);

  copy->start_step_no = orig->cursteps;

  copy->elim_stack = initialize_ri_stack();
  copy->finished = false;
  copy->parent = orig;
  return copy;
}

/**
   Called from prover.c when treating a disjunction which turns out not to be used. 
   The information about the end point, and eliminations in the state "below" in the proof tree is
   transferred to the state above

   The endpoint of the parent is overwritten with that in the child

   The step numbering is also transferred, since this is necessary for correct coq output.

   The parent state cannot be used for anything else than printing output after this, as
   the queues etc. are not correct anymore.
**/
void transfer_state_endpoint(rete_net_state* parent, rete_net_state* child){ 
  parent->end_of_branch = child->end_of_branch;
  add_ri_stack(parent->elim_stack, child->elim_stack);
  parent->branches = child->branches;

  parent->step_no = child->step_no;
  parent->cursteps = child->cursteps;
  
}

/**
   For testing whether a conjunction with some substitutions already done, is true in the fact set

   Note that sub is changed if it returns true.
**/
bool remaining_conjunction_true_in_fact_set(rete_net_state* state, const conjunction* con, unsigned int conjunct, substitution* sub){
  const fact_set * fs;
  if(conjunct >= con->n_args)
    return true;
  fs = state->factset[con->args[conjunct]->pred->pred_no];
  while(fs != NULL){
    substitution* sub2 = copy_substitution(sub, & state->local_subst_mem, state->net->th->sub_size_info);
    if(find_instantiate_sub(con->args[conjunct], fs->fact, sub2, state->constants)){
      if(remaining_conjunction_true_in_fact_set(state, con, conjunct+1, sub2)){
	return true;
      }
    }
    fs = fs->next;
  }
  return false;
}

/**
   Returns true if the conjunction is true in the factset.
   In that case, the given substitution is extended to such an instance.
   sub must have been instantiated and later freed by the calling function.
**/
bool conjunction_true_in_fact_set(rete_net_state* state, const conjunction* con, substitution* sub){
  return remaining_conjunction_true_in_fact_set(state, con, 0, sub);
}

bool disjunction_true_in_fact_set(rete_net_state* state, const disjunction* dis, substitution* sub){
  int i;
  for(i = 0; i < dis->n_args; i++){
    if(conjunction_true_in_fact_set(state, dis->args[i], sub))
      return true;
  }
  return false;
}

/**
   returns true if the axiom has an instance such that the lhs is true 
   in the factset, while the rhs is false. If this is indeed the case, 
   then sub is instantiated with the corresponding substitution

   The calling function must in that case free sub.
**/
bool remaining_axiom_false_in_fact_set(rete_net_state* state, 
				       const axiom* axm, 
				       size_t arg_no, 
				       substitution** sub){
  fact_set * fs;
  size_t pred_no;
  if(arg_no >= axm->lhs->n_args)
    return !disjunction_true_in_fact_set(state, axm->rhs, *sub);
  pred_no = axm->lhs->args[arg_no]->pred->pred_no;
  fs = state->factset[pred_no];
  if(fs == NULL)
    return false;

  fs->prev = NULL;
  while(fs->next != NULL){
    fs->next->prev = fs;
    fs = fs->next;
  }
  do{
    substitution* sub2 = copy_substitution(*sub, & state->local_subst_mem, state->net->th->sub_size_info);
    if(find_instantiate_sub(axm->lhs->args[arg_no], fs->fact, sub2, state->constants)){
      add_sub_timestamp(sub2, get_fact_set_timestamp(fs), state->net->th->sub_size_info);
      if(remaining_axiom_false_in_fact_set(state, axm, arg_no+1, &sub2)){
	*sub = sub2;
	return true;
      }
    }
    fs = fs->prev;
  } while (fs != NULL);
  return false;
}

bool axiom_false_in_fact_set(rete_net_state* state, size_t axiom_no, substitution** sub){
  substitution* sub2 = create_empty_substitution(state->net->th, & state->local_subst_mem);
  
  if(remaining_axiom_false_in_fact_set(state, state->net->th->axioms[axiom_no], 0, &sub2)){
    *sub = sub2;
    return true;
  }
  return false;
}

/**
   All sub_list_iter _must_be freed with the according function below.
   Otherwise the threaded version will deadlock
**/
   
sub_list_iter* get_state_sub_list_iter(rete_net_state* state, unsigned int sub_no){
#ifdef HAVE_PTHREAD
  pthread_mutex_lock(& state->net->sub_mutexes[sub_no]);
#endif
  return get_sub_list_iter(state->subs[sub_no]);
}

void free_state_sub_list_iter(rete_net_state* state, unsigned int sub_no, sub_list_iter* i){
#ifdef HAVE_PTHREAD
  pthread_mutex_unlock(& state->net->sub_mutexes[sub_no]);
#endif
  free_sub_list_iter(i);
}
 

/**
   Registers increase in proof step counter

   Returns false if maximum number of steps 

   Also sets s->cursteps to the current step.
   The latter value is used by proof_writer.c:write_proof_edge for coq proofs

   s->cursteps is also used by get_global_step_no
**/
bool inc_proof_step_counter(rete_net_state* s){
#ifdef __GNUC__
  s->cursteps = __sync_add_and_fetch(s->global_step_counter, 1);
#else
  s->cursteps = * s->global_step_counter;
  (* s->global_step_counter) ++;
#endif
  s->step_no ++;
  return (s->net->maxsteps == 0 || s->cursteps < s->net->maxsteps);
}

bool proof_steps_limit(rete_net_state* s){
  return (s->net->maxsteps == 0 || (* s->global_step_counter) < s->net->maxsteps);
}

unsigned int get_current_state_step_no(const rete_net_state* s){
  return s->cursteps;
}

unsigned int get_latest_global_step_no(const rete_net_state* s){
  return * (s->global_step_counter);
}

/**
   Printing the state
**/
void print_rete_state(const rete_net_state* state, FILE*  f){
  unsigned int i;
  fprintf(f, "State of RETE net\n");
  for(i = 0; i < state->net->th->n_axioms; i++){
    printf("Axiom %s ", state->net->th->axioms[i]->name);
    print_rule_queue(state->axiom_inst_queue[i], state->constants, f);
  }
  //  print_rule_queue(state->rule_queue, f);
  fprintf(f, "\nBranch: %s, Step: %i", state->proof_branch_id,state->step_no);
  fprintf(f, "\nSubstitution Lists:");
  for(i = 0; i < state->net->n_subs; i++){
    fprintf(f, "\n\t%i: ", i);
    if(state->subs[i] != NULL)
      print_substitution_list(state->subs[i], state->constants, f);
  }
  fprintf(f, "\n-----------------------------\n");
}


/**
   Called from proof_writer.c via prover.c
**/
void print_state_rule_queues(rete_net_state* s, FILE* f){
  unsigned int i;
  for(i = 0; i < s->net->th->n_axioms; i++){
    if(s->axiom_inst_queue[i]->end != s->axiom_inst_queue[i]->first){
      printf("Axiom %s ", s->net->th->axioms[i]->name);
      print_rule_queue(s->axiom_inst_queue[i], s->constants, stdout);
    }
  }
}

void print_dot_rete_state_node(const rete_node* node, const rete_net_state* state, FILE* stream){
  unsigned int i;
  fprintf(stream, "n%li [label=\"", (unsigned long) node);
  if(node->type == rule)
    print_dot_axiom(node->val.rule.axm, state->constants, stream);
  else
    print_rete_node_type(node, state->constants, stream);
  if(node->type == beta_and || node->type == beta_not){
    fprintf(stream, "\\nStore a: {");
    print_substitution_list(state->subs[node->val.beta.a_store_no], state->constants, stream);
    fprintf(stream, "}");
    fprintf(stream, "\\nStore b: {");
    print_substitution_list(state->subs[node->val.beta.b_store_no], state->constants, stream);
    fprintf(stream, "}");
  }
  fprintf(stream, "\"]\n");
  for(i = 0; i < node->n_children; i++){
    assert(node->children[i]->type != selector);
    fprintf(stream, "n%li -> n%li [label=%s]\n", 
	    (unsigned long) node, 
	    (unsigned long) node->children[i], 
	    (node->children[i]->left_parent == node) ? "left" : "right");
    print_dot_rete_state_node(node->children[i], state, stream);
  }
}


void print_dot_rete_state_net(const rete_net* net, const rete_net_state* state, FILE* stream){
  unsigned int i;
  fprintf(stream, "strict digraph RETESTATE {\n");
  for(i = 0; i < net->n_selectors; i++)
    print_dot_rete_state_node(& net->selectors[i], state, stream);
  fprintf(stream, "}\n");
}



/**
   Printing the facts that were inserted in this proof step
**/
void print_state_new_facts(rete_net_state* state, FILE* f){
  int i;
  for(i = 0; i < state->net->th->n_predicates; i++){
    fact_set* fs = state->factset[i];
    fact_set* old = state->prev_factset[i];
    bool first_iter = true;
    while(fs != NULL && fs != old){
      if(!first_iter)
	fprintf(f, ", ");
      first_iter = false;
      print_fol_atom(fs->fact, state->constants, f);
      fs = fs->next;
    }
    state->prev_factset[i] = state->factset[i];
  }
}
  
