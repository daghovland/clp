/* rete_state_single_struct.h

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
#ifndef __INCLUDED_RETE_STATE_SINGLE_STRUCT_H
#define __INCLUDED_RETE_STATE_SINGLE_STRUCT_H

#include "common.h"
#include "rete_net.h"
#include "constants.h"
#include "substitution_store.h"
#include "rule_queue_single.h"
#include "fact_store.h"
#include "sub_alpha_queue.h"

/**
   This version of a rete state is intended for a prover without or-parallellism, but
   with parallellism in the insertion into the rule queues

   subs is the substitutions stored in the node caches in the rete net

   new_facts_iters is used to keep track of what are the new facts at each step
**/
typedef struct rete_state_single_t {
  substitution_store * subs;
  substitution_store_mt tmp_subs;
  rule_queue_single ** rule_queues;
  rule_queue_single * history;
  sub_alpha_queue * worker_queues;
  fact_store * factsets;
  fact_store_iter * new_facts_iters;
  const rete_net* net;
  fresh_const_counter fresh;  
  constants constants;
  bool verbose;
  bool finished;
  unsigned int step;
} rete_state_single;


/**
   Stores information needed for restoring the rete state at a disjunction/splitting point
**/
typedef struct rete_state_backup_t {
  rule_queue_single_backup * rq_backups;
  substitution_store_backup * sub_backups;
  fact_store_backup * factset_backups;
  fact_store_iter * new_facts_backups;
  rete_state_single* state;
} rete_state_backup;

#endif