/* rete_insert_single.h

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
#ifndef __INCLUDED_RETE_INSERT_SINGLE_H
#define __INCLUDED_RETE_INSERT_SINGLE_H

/**
   Code for updating state of rete network
**/
#include "common.h"
#include "theory.h"
#include "substitution_store_array.h"
#include "substitution_store_mt.h"
#include "rete_node.h"
#include "atom.h"

bool insert_rete_alpha_fact_single(const rete_net* net,
				   substitution_store_array * node_caches, 
				   substitution_store_mt * tmp_subs,
				   timestamp_store* ts_store, 
				   rule_queue_single * rule_queue,
				   const rete_node* node, 
				   const clp_atom* fact, 
				   unsigned int step, 
				   substitution* sub, 
				   constants*);

void recheck_beta_node(const rete_net* net,
		       substitution_store_array * node_caches, 
		       substitution_store_mt * tmp_subs,
		       timestamp_store* ts_store, 
		       const rete_node* node, 
		       rule_queue_single * rule_queue,
		       unsigned int step, 
		       constants* cs);
#endif
