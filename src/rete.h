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
#include "rete_node.h"
#include "sub_alpha_queue.h"
#include "rete_net.h"

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

// Called from prover.c
void insert_rete_net_fact(rete_net_state*, const atom*);

//Called from lazy_rule_queue.c
void insert_rete_alpha_fact_children(rete_net_state*, const rete_node*, const atom*, substitution*, bool);

// Creates root of network
// Initializes the rete_net_state structure
rete_net* init_rete(const theory*, unsigned long, bool lazy, bool coq);

/** 
    Creates a "copy" of a rete net state
    for threading. 
**/
rete_net_state* split_rete_state(const rete_net_state*, size_t);

bool inc_proof_step_counter(rete_net_state*);

rete_node* create_beta_left_root(size_t axiom_no);

rete_node* create_alpha_node(rete_node*, unsigned int, const term*, const freevars*, bool propagate, size_t axiom_no);
/*rete_node* create_store_node(rete_net*, rete_node*, const freevars*);*/
rete_node* create_beta_and_node(rete_net*, rete_node*, rete_node*, const freevars*, size_t axiom_no);
rete_node* create_beta_or_node(rete_net*, rete_node*, rete_node*, const freevars*, size_t axiom_no);
rete_node* create_beta_not_node(rete_net*, rete_node*, rete_node*, const freevars*, size_t axiom_no);
rete_node* create_selector_node(rete_net*, const char*, unsigned int, const freevars*);
void create_rule_node(rete_net*, rete_node*, const axiom*, const freevars*, size_t axiom_no);

rete_net_state* create_rete_state(const rete_net*, bool);

// state is deleted. orig is the "split point" above state, which should be kept.
void delete_rete_state(rete_net_state* state, rete_net_state* orig);

// Called at the end of prover in prover.c
void delete_full_rete_state(rete_net_state* state);

// In sub_alpha_queue.c, called from strategy.c
bool axiom_has_new_instance(size_t axiom_no, rete_net_state * state);
bool axiom_may_have_new_instance(size_t axiom_no, rete_net_state* state);
unsigned int rule_queue_possible_age(size_t axiom_no, rete_net_state* state);

sub_list_iter* get_state_sub_list_iter(rete_net_state*, size_t);
void free_state_sub_list_iter(rete_net_state*, size_t, sub_list_iter*);

rete_node* get_selector(size_t, rete_net*);
const rete_node* get_const_selector(size_t, const rete_net*);

// Updates network with possibly new predicate name, returns the bottom alpha node for this atom
rete_node* create_rete_atom_node(rete_net*, const atom*, const freevars*, bool propagate, size_t axiom_no);
void create_rete_axiom_node(rete_net*, const axiom*, size_t axiom_no, bool);
rete_net* create_rete_net(const theory*, unsigned long, bool, strategy, bool, bool, bool, bool, bool, bool);
rete_node* create_rete_conj_node(rete_net*, const conjunction*, const freevars*, bool propagate, size_t axiom_no);
rete_node* create_rete_disj_node(rete_net*, rete_node*, const disjunction*, size_t axiom_no);

// Defined in strategy.c
rule_instance* choose_next_instance(rete_net_state*, strategy);

void add_rete_child(rete_node* parent, const rete_node* new_child);

void delete_rete_net(rete_net*);

// In prover.c
unsigned int prover(const rete_net*, bool);


bool test_rete_net(const rete_net*);
bool test_rule_instance(const rule_instance*, const rete_net_state*);
bool test_rule_queue(const rule_queue*, const rete_net_state*);


void print_rete_net(const rete_net*, FILE* );
void print_dot_rete_net(const rete_net*, FILE* );
void print_state_new_facts(rete_net_state*, FILE*);
void print_rete_node_type(const rete_node*, FILE*);


rule_instance* pop_axiom_rule_queue(rete_net_state*, size_t);
rule_instance* pop_youngest_axiom_rule_queue(rete_net_state*, size_t);
rule_instance* peek_axiom_rule_queue(const rete_net_state*, size_t);
void remove_rule_instance(rete_net_state*, const substitution*, size_t);
void add_rule_to_queue(const axiom*, substitution*, rete_net_state*);
bool is_empty_axiom_rule_queue(rete_net_state*, size_t);

const term* find_substitution(const substitution*, const variable*);

// In instantiate.c
atom* instantiate_atom(const atom*, const substitution*);
void delete_instantiated_atom(const atom*, atom*);
const term_list* instantiate_term_list(const term_list*, const substitution*);
void delete_instantiated_term_list(const term_list*, term_list*);
const term* instantiate_term(const term*, const substitution*);
void delete_instantiated_term(const term*, term*);

const term* get_fresh_constant(rete_net_state*, variable*);

void print_rete_state(const rete_net_state*, FILE*);
void print_dot_rete_state_net(const rete_net*, const rete_net_state*, FILE*);

bool insert_substitution(rete_net_state*, size_t, substitution*, const freevars*);

void insert_state_fact_set(rete_net_state*, const atom*);

// in rete_state.c
unsigned int get_current_state_step_no(const rete_net_state*);
unsigned int get_latest_global_step_no(const rete_net_state*);
bool proof_steps_limit(rete_net_state* s);
void fresh_exist_constants(rete_net_state*, const conjunction*, substitution*);
#endif
