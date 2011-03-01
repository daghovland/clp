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

void init_proof_dot_writer(const char*);
void write_proof_node(rete_net_state*, const rule_instance*);
void end_proof_dot_writer(const char*);
void write_proof_state(const rete_net_state*, const rete_net_state*);
void write_proof_edge(const rete_net_state*, const rete_net_state*);
#endif


