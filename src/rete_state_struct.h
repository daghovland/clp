/* rete_state_struct.h

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
#ifndef __INCLUDED_RETE_STATE_STRUCT_H
#define __INCLUDED_RETE_STATE_STRUCT_H

#include "common.h"
#include "rete_net.h"
#include "constants.h"
#include "substitution_store_mt.h"
#include "sub_alpha_queue.h"
#include "rule_instance_stack.h"


/**
   The state of the rete net changes during the run of the prover
   It is not completely shared between threads
   
   The substitution lists are the contents of the alpha and beta stores
   The val.beta.a_store_no,  val.beta.b_store_no, and val.rule.store_no in rete_node are 
   indices into this list
   
   The rule_queue and axiom_inst_queue[] is the queue of applicable rule instances

   There is also a queue of pointers for each axiom

   thread_id is a string describing the branch in the proof tree.
   With a branch is here meant a part of the tree that is a path. 
   step_no is the step number along this branch

   old_fact_set is used to print out only the newly inserted facts

   elim_stack is used for the coq proof output.
   It is pushed in proof_writer.c:write_elim_usage_proof

   finished is true when the branch is done

   The size of "branches" equals end_of_branch->rule->rhs->n_args

   The fact_set is an array of length net->th->n_predicates containing the substitutions
   of the respective atoms

   local_subst_mem is used for most of the substitutions. These are all deleted
   when a branch is done.

   global_subst_mem is used for the substitutions that need to be saved to reconstruct the coq proof.
**/
typedef struct rete_net_state_t {
  const char* proof_branch_id;
  unsigned int step_no;
  substitution_store_mt local_subst_mem;
  substitution_store_mt* global_subst_mem;
  substitution_list ** subs;
  sub_alpha_queue * sub_alpha_queues;
  bool * is_applied_fact;
  const rete_net* net;
  unsigned int * global_step_counter;
  unsigned int cursteps;
  unsigned int start_step_no;
  constants* constants;
  bool verbose;
  rule_instance* end_of_branch;
  rule_instance_stack* elim_stack;
  fact_set ** factset;
  fact_set ** prev_factset;
  struct rete_net_state_t ** branches;
  struct rete_net_state_t * parent;
  bool finished;
  rule_queue* axiom_inst_queue[];
} rete_net_state;


#endif
