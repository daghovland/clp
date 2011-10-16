/* rete_state.h

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
#ifndef __INCLUDED_RETE_STATE_H
#define __INCLUDED_RETE_STATE_H

#include "common.h"
#include "rete_state_struct.h"
#include "rete_net.h"
#include "constants.h"
#include "substitution.h"
#include "rule_queue_state.h"

// In rete_state.c

void transfer_state_endpoint(rete_net_state* parent, rete_net_state* child);
bool conjunction_true_in_fact_set(rete_net_state* state, const conjunction* con, substitution* sub);
bool disjunction_true_in_fact_set(rete_net_state* state, const disjunction* dis, substitution* sub);
bool axiom_false_in_fact_set(rete_net_state* state, size_t axiom_no, substitution** sub);

rule_instance* choose_next_instance_state(rete_net_state*);
void print_state_rule_queues(rete_net_state*, FILE*);
#endif
