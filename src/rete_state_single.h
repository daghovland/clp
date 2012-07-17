/* rete_state_single.h

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
#ifndef __INCLUDED_RETE_STATE_SINGLE_H
#define __INCLUDED_RETE_STATE_SINGLE_H

#include "strategy.h"
#include "rete_state_single_struct.h"
#include "rule_instance.h"
#include "rete_net.h"
#include "rule_queue_state.h"


rete_state_single* create_rete_state_single(const rete_net*, bool);
void stop_rete_state_single(rete_state_single*);
void delete_rete_state_single(rete_state_single*);
void pause_rete_state_workers(rete_state_single*);
bool test_rete_state(rete_state_single*);

rule_instance* choose_next_instance_single(rete_state_single*);
rule_instance* insert_rule_instance_history_single(rete_state_single* state, const rule_instance*);
rete_state_backup backup_rete_state(rete_state_single*);
void destroy_rete_backup(rete_state_backup*);
void restore_rete_state(rete_state_backup*, rete_state_single*);

void check_used_rule_instances_coq_single(rule_instance*, rete_state_single*, proof_branch*, timestamp, timestamp);
rule_instance* get_historic_rule_instance(rete_state_single*, unsigned int);
void enter_proof_disjunct(rete_state_single*);

bool insert_state_factset_single(rete_state_single*, const clp_atom*);

unsigned int get_state_step_no_single(const rete_state_single*);
unsigned int get_state_total_steps(const rete_state_single*);
bool inc_proof_step_counter_single(rete_state_single*);

sub_store_iter get_state_sub_store_iter(rete_state_single*, unsigned int);
fact_store_iter get_state_fact_store_iter(rete_state_single*, unsigned int);

void insert_rete_worker_queue(rete_state_single*, substitution*, const clp_atom*, const rete_node*);
void recheck_rete_state_net(rete_state_single*);

void insert_state_rete_net_fact(rete_state_single* state, const clp_atom* fact);

void print_state_single_rule_queues(rete_state_single*, FILE*);
void print_state_fact_store(rete_state_single *, FILE*);
void print_state_new_fact_store(rete_state_single*, FILE*);
#endif
