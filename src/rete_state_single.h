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
#include "rule_instance_single.h"
#include "rete_net.h"
#include "rule_queue_state.h"

rete_state_single* create_rete_state_single(const rete_net*, bool);
void delete_rete_state_single(rete_state_single*);

rule_instance_single* choose_next_instance_single(rete_state_single*);
rete_state_backup backup_rete_state(rete_state_single*);
void destroy_rete_backup(rete_state_backup*);
rete_state_single* restore_rete_state(rete_state_backup*);


bool is_empty_axiom_rule_queue_single(rule_queue_state, size_t);
rule_instance_union peek_axiom_rule_queue_single(rule_queue_state, size_t);
bool axiom_has_new_instance_single(rule_queue_state, size_t);
unsigned int rule_queue_possible_age_single(rule_queue_state, size_t);
bool axiom_may_have_new_instance_single(rule_queue_state, size_t);
rule_instance_union pop_axiom_rule_queue_single(rule_queue_state, size_t);
void add_rule_to_queue_single(const axiom*, substitution*, rule_queue_state);
unsigned int axiom_queue_previous_application_single(rule_queue_state, size_t);

unsigned int get_state_step_single(const rete_state_single*);
bool inc_proof_step_counter_single(rete_state_single*);
#endif
