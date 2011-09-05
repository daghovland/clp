/* proof_writer.h

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
/**
   Writes out the proof to a dot-file, or something else
**/

#ifndef __INCLUDED_PROOF_WRITER_H
#define __INCLUDED_PROOF_WRITER_H

#include "rete.h"
#include "rule_queue.h"
#include "rule_instance_state_stack.h"

FILE* get_coq_fdes(void);
bool next_ri_is_after(rule_instance_stack*, unsigned int);
void init_proof_dot_writer(const char*, const rete_net*);
void init_proof_coq_writer(const char*, const rete_net*);
void write_proof_node(rete_net_state*, const rule_instance*);
void end_proof_dot_writer(const char*, const rete_net*);
void end_proof_coq_writer(const theory* th);
void write_proof_state(const rete_net_state*, const rete_net_state*);
void write_proof_edge(const rete_net_state*, const rete_net_state*);
void write_goal_proof(const rule_instance*, const rete_net_state*, int, rule_instance_state**);
void write_elim_usage_proof(rete_net_state*, rule_instance*, int);
void write_disj_proof_start(const rule_instance* ri, int ts, int branch);
void write_premiss_proof(const rule_instance* ri, int ts, rule_instance_state**);
#endif


