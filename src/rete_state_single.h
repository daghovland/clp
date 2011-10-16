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
void delete_rete_state_single(rete_state_single*);

rule_instance* choose_next_instance_single(rete_state_single*);
rete_state_backup backup_rete_state(rete_state_single*);
void destroy_rete_backup(rete_state_backup*);
rete_state_single* restore_rete_state(rete_state_backup*);

void insert_state_factset_single(rete_state_single*, const atom*);

unsigned int get_state_step_single(const rete_state_single*);
bool inc_proof_step_counter_single(rete_state_single*);

sub_store_iter get_state_sub_store_iter(rete_state_single*, unsigned int);
fact_store_iter get_state_fact_store_iter(rete_state_single*, unsigned int);

void print_state_single_rule_queues(rete_state_single*, FILE*);
void print_state_fact_store(rete_state_single *, FILE*);
void print_state_new_fact_store(rete_state_single*, FILE*);
#endif
