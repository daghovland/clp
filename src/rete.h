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
#include "rete_state_struct.h"
#include "rule_queue_state.h"
#include "strategy.h"
#include "rule_instance.h"

typedef unsigned int domain_iter;
void insert_domain_elem(rete_net_state*, const clp_term*);
domain_iter get_domain_iter(rete_net_state*);
bool domain_iter_has_next(rete_net_state*, domain_iter*);
const clp_term* domain_iter_get_next(rete_net_state*, domain_iter*);
void insert_constants_elem(rete_net_state*, const clp_term*);

// Called from prover.c
void insert_rete_net_fact(rete_net_state*, const clp_atom*);



//Called from lazy_rule_queue.c
void insert_rete_alpha_fact_children(rete_net_state*, const rete_node*, const clp_atom*, substitution*, bool);

// Creates root of network
// Initializes the rete_net_state structure
rete_net* init_rete(const theory*, unsigned long, bool lazy, bool coq, bool multithread_rete);

/** 
    Creates a "copy" of a rete net state
    for threading. 
**/
rete_net_state* split_rete_state(rete_net_state*, unsigned int);

bool inc_proof_step_counter(rete_net_state*);

rete_node* create_beta_left_root(unsigned int axiom_no);

rete_node* create_alpha_node(rete_node*, unsigned int, const clp_term*, const freevars*, bool propagate, bool  in_positive_lhs_part, unsigned int axiom_no);
/*rete_node* create_store_node(rete_net*, rete_node*, const freevars*);*/
rete_node* create_beta_and_node(rete_net*, rete_node*, rete_node*, const freevars*, bool in_positive_lhs_part, unsigned int axiom_no);
rete_node* create_beta_or_node(rete_net*, rete_node*, rete_node*, const freevars*, bool in_positive_lhs_part, unsigned int axiom_no);
rete_node* create_beta_not_node(rete_net*, rete_node*, rete_node*, const freevars*, unsigned int axiom_no);
rete_node* create_selector_node(rete_net*, const char*, unsigned int, const freevars*);
rete_node* create_rule_node(rete_net*, rete_node*, const clp_axiom*, const freevars*, unsigned int axiom_no);
rete_node* create_rete_equality_node(rete_net* , const clp_term* , const clp_term* , rete_node* , const freevars*, bool , unsigned int);

rete_net_state* create_rete_state(const rete_net*, bool);

// state is deleted. orig is the "split point" above state, which should be kept.
void delete_rete_state(rete_net_state* state, rete_net_state* orig);

// Called at the end of prover in prover.c
void delete_full_rete_state(rete_net_state* state);

// In sub_alpha_queue.c, called from strategy.c
bool axiom_has_new_instance(rule_queue_state, unsigned int);
bool axiom_may_have_new_instance(rule_queue_state, unsigned int);
unsigned int rule_queue_possible_age(rule_queue_state, unsigned int);

sub_list_iter* get_state_sub_list_iter(rete_net_state*, unsigned int);
void free_state_sub_list_iter(rete_net_state*, unsigned int, sub_list_iter*);

rete_node* get_selector(unsigned int, rete_net*);
const rete_node* get_const_selector(unsigned int, const rete_net*);

// Updates network with possibly new predicate name, returns the bottom alpha node for this atom
rete_node* create_rete_atom_node(rete_net*, const clp_atom*, const freevars*, bool propagate, bool in_positive_lhs_part, unsigned int axiom_no);
rete_node* create_rete_axiom_node(rete_net*, const clp_axiom*, unsigned int axiom_no, bool);
rete_net* create_rete_net(const theory*, unsigned long, bool, strategy, bool, bool, bool, bool, bool, bool, bool, bool);
rete_node* create_rete_conj_node(rete_net*, const clp_conjunction*, const freevars*, bool propagate, bool in_postive_lhs_part, unsigned int axiom_no);
rete_node* create_rete_disj_node(rete_net*, rete_node*, const clp_disjunction*, unsigned int axiom_no);
rete_node * insert_beta_not_nodes(rete_net* net, const clp_conjunction* con, const clp_disjunction* dis, rete_node* beta_node, unsigned int axiom_no);

bool true_ground_equality(const clp_term*, const clp_term*, const substitution*, constants*, timestamps*, timestamp_store*, bool update_ts);

// Defined in strategy.c
rule_instance* choose_next_instance(rule_queue_state
				    , const rete_net*
				    , strategy
				    , unsigned int
				    , bool (*) (rule_queue_state, unsigned int)
				    , rule_instance* (*)(rule_queue_state, unsigned int)
				    , bool (*has_new_instance)(rule_queue_state, unsigned int)
				    , unsigned int (*possible_age)(rule_queue_state, unsigned int)
				    , bool (*may_have)(rule_queue_state, unsigned int)
				    , rule_instance* (*pop_axiom)(rule_queue_state, unsigned int)
				    , void (*) (const clp_axiom*, const substitution*, rule_queue_state)
				    , unsigned int (*previous_application)(rule_queue_state, unsigned int)
				    );



void add_rete_child(rete_node* parent, const rete_node* new_child);

void delete_rete_net(rete_net*);

// In prover.c
unsigned int prover(const rete_net*, bool);
// In prover_single.c
unsigned int prover_single(const rete_net*, bool);


bool test_rete_net(const rete_net*);
bool test_rule_queue(const rule_queue*, const rete_net_state*);


void print_rete_net(const rete_net*, const constants*, FILE* );
void print_dot_rete_net(const rete_net*, const constants*, FILE* );
void print_state_new_facts(rete_net_state*, FILE*);
void print_rete_node_type(const rete_node*, const constants*, FILE*);
void print_rete_node(const rete_node*, const constants*, FILE*, unsigned int);


// In rule_queue.c and sub_alpha_queue_c

substitution* get_rule_instance_single_subsitution(rule_instance*);
substitution* get_rule_instance_subsitution(rule_instance*);
unsigned int axiom_queue_previous_application(rule_queue_state, unsigned int);
rule_instance* pop_axiom_rule_queue(rete_net_state*, unsigned int);
rule_instance* pop_axiom_rule_queue_state(rule_queue_state, unsigned int);
rule_instance* pop_youngest_axiom_rule_queue(rule_queue_state, unsigned int);
rule_instance* peek_axiom_rule_queue_state(rule_queue_state, unsigned int);
rule_instance* peek_axiom_rule_queue(rete_net_state*, unsigned int);
void remove_rule_instance(rete_net_state*, const substitution*, unsigned int);
void add_rule_to_queue_state(const clp_axiom*, const substitution*, rule_queue_state);
void add_rule_to_queue(const clp_axiom*, const substitution*, rete_net_state*);
bool is_empty_axiom_rule_queue(rete_net_state*, unsigned int);
bool is_empty_axiom_rule_queue_state(rule_queue_state, unsigned int);

void print_rete_state(const rete_net_state*, FILE*);
void print_dot_rete_state_net(const rete_net*, const rete_net_state*, FILE*);

bool insert_substitution(rete_net_state*, unsigned int, substitution*, const freevars*, constants*);

// in rete_state.c
unsigned int get_current_state_step_no(const rete_net_state*);
unsigned int get_latest_global_step_no(const rete_net_state*);
bool proof_steps_limit(rete_net_state* s);
#endif
