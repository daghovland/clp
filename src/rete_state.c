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

#include "common.h"
#include "rete.h"


/**
   Called after the rete net is created.
   Initializes the substition lists and queues in the state
**/
rete_net_state* create_rete_state(const rete_net* net, bool verbose){
  unsigned int i;
  rete_net_state* state = malloc_tester(sizeof(rete_net_state) 
					+ (net->th->n_axioms * sizeof(rule_queue*)));
  state->subs = calloc_tester(sizeof(substitution_list*), net->n_subs);
  state->sub_alpha_queues = calloc_tester(sizeof(sub_alpha_queue*), net->th->n_axioms);
					  
  state->verbose = verbose;
  for(i = 0; i < net->n_subs; i++){
    state->subs[i] = NULL;
  }
  state->net = net;
  state->fresh = init_fresh_const(net->th->vars->n_vars);
  assert(state->fresh != NULL);
  state->facts = create_fact_set();
  state->rule_queue = initialize_queue();
  for(i = 0; i < net->th->n_axioms; i++)
    state->axiom_inst_queue[i] = initialize_queue();
  state->step_no = 0;
  state->global_step_counter = malloc_tester(sizeof(unsigned int));
  * state->global_step_counter = 0;
  state->proof_branch_id = "";
  state->old_fact_set = NULL;

  state->size_domain = 2;
  state->domain = calloc_tester(state->size_domain, sizeof(term*));
  state->n_domain = 0;

  state->size_constants = 2;
  state->constants = calloc_tester(state->size_constants, sizeof(char*));
  state->n_constants = 0;
  return state;
}

void delete_rete_state(rete_net_state* state){
  unsigned int i;
  free(state->subs);
  delete_fact_set(state->facts);
  delete_rule_queue(state->rule_queue);
  for(i = 0; i < state->net->th->n_axioms; i++)
    free(state->axiom_inst_queue[i]);
  if(strlen(state->proof_branch_id) > 0)
    free((char *) state->proof_branch_id);
  free(state->domain);
  free(state->constants);
  free(state);
}

/**
   Copy "constructor"

   Used when treating a disjunctive rule

   size_t is the number of the branch, numbered from left to right, starting at 0
**/
rete_net_state* split_rete_state(const rete_net_state* orig, size_t branch_no){
  unsigned int i;
  size_t orig_size = sizeof(rete_net_state) + orig->net->th->n_axioms * sizeof(rule_queue*);
  size_t n_subs = orig->net->n_subs;
  size_t branch_id_len;

  rete_net_state* copy = malloc_tester(orig_size);

  memcpy(copy, orig, orig_size);
  copy->subs = calloc_tester(n_subs, sizeof(substitution_list*));
  memcpy(copy->subs, orig->subs, sizeof(substitution_list*) * n_subs);

  assert(copy->n_domain == orig->n_domain);
  copy->domain = calloc_tester(orig->size_domain, sizeof(term*));
  memcpy(copy->domain, orig->domain, orig->size_domain * sizeof(term*));


  assert(copy->n_constants == orig->n_constants);
  copy->constants = calloc_tester(orig->size_constants, sizeof(char*));
  memcpy(copy->constants, orig->constants, orig->size_constants * sizeof(char*));

  copy->rule_queue = copy_rule_queue(orig->rule_queue);
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
  assert(test_rule_queue(copy->rule_queue, copy));
  assert(orig->global_step_counter == copy->global_step_counter);
  split_fact_set(orig->facts);
  return copy;
}

/**
   Inserts a fact into the fact set
**/

void insert_state_fact_set(rete_net_state* s, const atom* a){
  s->facts = insert_fact_set(s->facts, a);
}

sub_list_iter* get_state_sub_list_iter(rete_net_state* state, size_t sub_no){
  return get_sub_list_iter(state->subs[sub_no]);
}

/**
   We keep track of all constants, so equality on constants can be decided by pointer equality
**/
void insert_constant_name(rete_net_state * state, const char* name){
  unsigned int i;
  assert(name != NULL && strlen(name) > 0);
  for(i = 0; i < state->n_constants; i++){
    assert(state->constants[i] != NULL);
    if(strcmp(state->constants[i], name) == 0)
      return;
  }
  state->n_constants++;
  if(state->size_constants <= state->n_constants){
    state->size_constants *= 2;
    state->constants = realloc_tester(state->constants, state->size_constants * sizeof(char*));
  }
  state->constants[state->n_constants - 1] = malloc_tester(strlen(name)+2);
  strcpy((char*) state->constants[state->n_constants - 1], name);
  return;
}

constants_iter get_constants_iter(rete_net_state* state){
  return 0;
}

bool constants_iter_has_next(rete_net_state* state, constants_iter* iter){
  return *iter < state->n_constants;
}

const char* constants_iter_get_next(rete_net_state* state, constants_iter *iter){
  assert(constants_iter_has_next(state, iter));
  return state->constants[*iter++];
}
  

/**
   The domain is governed by the special predicate dom
**/
void insert_domain_elem(rete_net_state * state, const term* trm){
  unsigned int i;
  assert(test_term(trm));
  for(i = 0; i < state->n_domain; i++){
    assert(state->domain[i] != NULL);
    if(equal_terms(state->domain[i], trm))
      return;
  }
  state->n_domain++;
  if(state->size_domain <= state->n_domain){
    state->size_domain *= 2;
    state->domain = realloc_tester(state->domain, state->size_domain * sizeof(char*));
  }
  state->domain[state->n_domain - 1] = trm;
  return;
}

domain_iter get_domain_iter(rete_net_state* state){
  return 0;
}

bool domain_iter_has_next(rete_net_state* state, domain_iter* iter){
  return *iter < state->n_domain;
}

const term* domain_iter_get_next(rete_net_state* state, domain_iter *iter){
  assert(domain_iter_has_next(state, iter));
  return state->domain[*iter++];
}
  

/**
   Registers increase in proof step counter

   Returns false if maximum number of steps 
**/
bool inc_proof_step_counter(rete_net_state* s){
#ifdef __GNUC__
  unsigned int cursteps = __sync_add_and_fetch(s->global_step_counter, 1);
#else
  unsigned int cursteps = * s->global_step_counter;
  (* s->global_step_counter) ++;
#endif
  s->step_no ++;
  return (s->net->maxsteps == 0 || cursteps < s->net->maxsteps);
}

bool proof_steps_limit(rete_net_state* s){
  return (s->net->maxsteps == 0 || (* s->global_step_counter) < s->net->maxsteps);
}

unsigned int get_global_step_no(const rete_net_state* s){
  return * s->global_step_counter;
}

/**
   The fresh constants
**/
const term* get_fresh_constant(rete_net_state* state, variable* var){
  char* name;
  const term* t;
  assert(var->var_no < state->net->th->vars->n_vars);
  assert(state->fresh != NULL);
  unsigned int const_no = next_fresh_const_no(state->fresh, var->var_no);
  name = calloc_tester(sizeof(char), strlen(var->name) + 20);
  sprintf(name, "%s_%i", var->name, const_no);
  t = prover_create_constant_term(name);
  insert_constant_name(state, name);
  insert_domain_elem(state, t);
  return t;
}

/**
   Printing the state
**/
void print_rete_state(const rete_net_state* state, FILE*  f){
  unsigned int i;
  fprintf(f, "State of RETE net\n");
  for(i = 0; i < state->net->th->n_axioms; i++)
    print_rule_queue(state->axiom_inst_queue[i], f);
  //  print_rule_queue(state->rule_queue, f);
  fprintf(f, "\nBranch: %s, Step: %i", state->proof_branch_id,state->step_no);
  fprintf(f, "\nSubstitution Lists:");
  for(i = 0; i < state->net->n_subs; i++){
    fprintf(f, "\n\t%i: ", i);
    if(state->subs[i] != NULL)
      print_substitution_list(state->subs[i], f);
  }
  fprintf(f, "\n-----------------------------\n");
}


void print_dot_rete_state_node(const rete_node* node, const rete_net_state* state, FILE* stream){
  unsigned int i;
  fprintf(stream, "n%li [label=\"", (unsigned long) node);
  if(node->type == rule)
    print_dot_axiom(node->val.rule.axm, stream);
  else
    print_rete_node_type(node, stream);
  if(node->type == beta_and || node->type == beta_not){
    fprintf(stream, "\\nStore a: {");
    print_substitution_list(state->subs[node->val.beta.a_store_no], stream);
    fprintf(stream, "}");
    fprintf(stream, "\\nStore b: {");
    print_substitution_list(state->subs[node->val.beta.b_store_no], stream);
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
  fact_set* fs = state->facts;
  if(fs != NULL && fs != state->old_fact_set){
    assert(test_atom(fs->fact));
    print_fol_atom(fs->fact, f);
    fs = fs->next;
    while(fs != NULL && fs != state->old_fact_set){
      fprintf(f, ", ");
      assert(test_atom(fs->fact));
      print_fol_atom(fs->fact, f);
      fs = fs->next;
    }
  }
  state->old_fact_set = state->facts;
}
  
