/* rete.h

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
#ifndef __INCLUDED_RETE
#define __INCLUDED_RETE

#include "predicate.h"
#include "term.h"
#include "atom.h"
#include "conjunction.h"
#include "disjunction.h"
#include "axiom.h"
#include "theory.h"
#include "substitution.h"
#include "rule_queue.h"
#include "fresh_constants.h"
#include "fact_set.h"
#include "strategy.h"

/**
   There is one predicate node for each predicate name occurring in the theory

   alpha nodes test intravariable constraints
   value nodes test that a variable has a specific term value
   bottom are special nodes with beta_node children. 
   All other nodes have only alpha_node children

 beta nodes test that different atoms have the same values
 where they are supposed to 

 beta_and and beta_not nodes have a right-parent (two-input-nodes)
 
 alpha nodes test argument number being unifiable with the term at that position
**/
enum rete_node_type { alpha, beta_and, beta_root, beta_not, selector, rule };

struct rete_node_t {
  enum rete_node_type type;
  union {
    predicate * selector;
    struct alpha_t {
      const term* value;
      unsigned int argument_no;
    } alpha;
    struct beta_t {
      const struct rete_node_t* right_parent;
      size_t a_store_no;
      size_t b_store_no;
    } beta;
    struct rule_t {
      const axiom* axm;
      size_t store_no;
    } rule;
  } val;
  const freevars* free_vars;
  const struct rete_node_t** children;
  int n_children;
  size_t size_children;
  const struct rete_node_t* left_parent;
};

typedef struct rete_node_t rete_node;

/**
   The root of the rete network is an array of rete_node's of type selector
   They must be sorted according to (1) the predicate name, (2) the arity
   There must be no duplicates in this sorting
   
   The root also contains the lists of substitutions for each alpha and beta store. 
   the value "store" in a store node is an index to subs in the root. This is to make 
   copying the substitutions easy when splitting into trees

   clpl is true if the commandline option --clpl is given. Leads to rule queues being sorted
   in the same way they would be by prolog
**/
typedef struct rete_net_t {
  size_t n_subs;
  size_t n_selectors;
  const theory* th;
  bool existdom;
  strategy strat;
  unsigned long maxsteps;
  rete_node selectors[];
} rete_net;





/**
   The state of the rete net changes during the run of the prover
   It is not completely shared between threads
   
   The substitution lists are the contents of the beta stores
   The val.beta.a_store_no,  val.beta.b_store_no, and val.rule.store_no in rete_node are 
   indices into this list
   
   The rule_queue and axiom_inst_queue[] is the queue of applicable rule instances

   There is also a queue of pointers for each axiom

   thread_id is a string describing the branch in the proof tree.
   With a branch is here meant a part of the tree that is a path. 
   step_no is the step number along this branch

   old_fact_set is used to print out only the newly inserted facts
**/
typedef struct rete_net_state_t {
  const char* proof_branch_id;
  unsigned int step_no;
  substitution_list ** subs;
  rule_queue* rule_queue;
  const rete_net* net;
  unsigned int * global_step_counter;
  fresh_const_counter fresh;
  fact_set* facts;
  fact_set* old_fact_set;
  const term** domain;
  size_t size_domain;
  unsigned int n_domain;
  const char** constants;
  size_t size_constants;
  unsigned int n_constants;
  bool verbose;
  rule_queue* axiom_inst_queue[];
} rete_net_state;
  
typedef unsigned int domain_iter;
typedef unsigned int constants_iter;
void insert_domain_elem(rete_net_state*, const term*);
domain_iter get_domain_iter(rete_net_state*);
bool domain_iter_has_next(rete_net_state*, domain_iter*);
const term* domain_iter_get_next(rete_net_state*, domain_iter*);
void insert_constants_elem(rete_net_state*, const term*);
constants_iter get_constants_iter(rete_net_state*);
bool constants_iter_has_next(rete_net_state*, constants_iter*);
const char* constants_iter_get_next(rete_net_state*, constants_iter*);

void insert_rete_net_fact(rete_net_state*, const atom*);


// Creates root of network
// Initializes the rete_net_state structure
rete_net* init_rete(const theory*, unsigned long);

/** 
    Creates a "copy" of a rete net state
    for threading. 
**/
rete_net_state* split_rete_state(const rete_net_state*, size_t);

bool inc_proof_step_counter(rete_net_state*);

rete_node* create_beta_left_root(void);

rete_node* create_alpha_node(rete_node*, unsigned int, const term*, const freevars*);
/*rete_node* create_store_node(rete_net*, rete_node*, const freevars*);*/
rete_node* create_beta_and_node(rete_net*, rete_node*, rete_node*, const freevars*);
rete_node* create_beta_not_node(rete_net*, rete_node*, rete_node*, const freevars*);
rete_node* create_selector_node(rete_net*, const char*, unsigned int, const freevars*);
void create_rule_node(rete_net*, rete_node*, const axiom*, const freevars*);

rete_net_state* create_rete_state(const rete_net*, bool);
void delete_rete_state(rete_net_state*);

sub_list_iter* get_state_sub_list_iter(rete_net_state*, size_t);

rete_node* get_selector(size_t, rete_net*);
const rete_node* get_const_selector(size_t, const rete_net*);

// Updates network with possibly new predicate name, returns the bottom alpha node for this atom
rete_node* create_rete_atom_node(rete_net*, const atom*, const freevars*);
void create_rete_axiom_node(rete_net*, const axiom*);
rete_net* create_rete_net(const theory*, unsigned long, bool, strategy);
rete_node* create_rete_conj_node(rete_net*, const conjunction*, const freevars*);
rete_node* create_rete_disj_node(rete_net*, rete_node*, const disjunction*);

// Defined in strategy.c
rule_instance* choose_next_instance(rete_net_state*, strategy);

void add_rete_child(rete_node* parent, const rete_node* new_child);

void delete_rete_net(rete_net*);

// In prover.c
unsigned int prover(const rete_net*, bool);


bool test_rete_net(const rete_net*);
bool test_rule_queue_sums(const rete_net_state*);
bool test_rule_instance(const rule_instance*, const rete_net_state*);
bool test_rule_queue(const rule_queue*, const rete_net_state*);



void print_rete_net(const rete_net*, FILE* );
void print_dot_rete_net(const rete_net*, FILE* );
void print_state_new_facts(rete_net_state*, FILE*);
void print_rete_node_type(const rete_node*, FILE*);


rule_instance* pop_rule_queue(rete_net_state*);
rule_instance* peek_rule_queue(const rete_net_state*);
rule_instance* pop_axiom_rule_queue(rete_net_state*, size_t);
rule_instance* pop_youngest_axiom_rule_queue(rete_net_state*, size_t);
rule_instance* peek_axiom_rule_queue(const rete_net_state*, size_t);
void remove_rule_instance(rete_net_state*, const substitution*, size_t);
void add_rule_to_queue(const axiom*, substitution*, rete_net_state*);

const term* find_substitution(const substitution*, const variable*);

// In instantiate.c
const atom* instantiate_atom(const atom*, const substitution*);
const term_list* instantiate_term_list(const term_list*, const substitution*);
const term* instantiate_term(const term*, const substitution*);

const term* get_fresh_constant(rete_net_state*, variable*);

void print_rete_state(const rete_net_state*, FILE*);
void print_dot_rete_state_net(const rete_net*, const rete_net_state*, FILE*);

bool insert_substitution(rete_net_state*, size_t, substitution*, const freevars*);

void insert_state_fact_set(rete_net_state*, const atom*);

// in rete_state.c
unsigned int get_global_step_no(const rete_net_state*);
bool proof_steps_limit(rete_net_state* s);
void fresh_exist_constants(rete_net_state*, const conjunction*, substitution*);
#endif
