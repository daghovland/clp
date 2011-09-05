/* rete_net.h

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
#ifndef __INCLUDED_RETE_NET
#define __INCLUDED_RETE_NET

#include "predicate.h"
#include "term.h"
#include "atom.h"
#include "conjunction.h"
#include "disjunction.h"
#include "axiom.h"
#include "theory.h"
#include "substitution.h"
#include "rule_queue.h"
#include "rule_instance_stack.h"
#include "fresh_constants.h"
#include "fact_set.h"
#include "strategy.h"
#include "rete_node.h"
#include "sub_alpha_queue.h"



/**
   The root of the rete network is an array of rete_node's of type selector
   They must be sorted according to (1) the predicate name, (2) the arity
   There must be no duplicates in this sorting
   
   The root also contains the lists of substitutions for each alpha and beta store. 
   the value "store" in a store node is an index to subs in the root. This is to make 
   copying the substitutions easy when splitting into trees

   strat is depth-first if the commandline option --depth-first is given. Leads to rule queues being sorted
   in the same way they would be by prolog in CL.pl

   history is used when printing coq format proofs. 
   Keeps track of used rule instances.
**/
typedef struct rete_net_t {
  size_t n_subs;
  size_t n_selectors;
  const theory* th;
  bool existdom;
  bool lazy;
  bool coq;
  strategy strat;

  //unsigned int size_history;
  // rule_instance ** history;

  unsigned long maxsteps;
  rete_node selectors[];
} rete_net;



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

   ri_stack is used for the coq proof output.
   It is pushed in proof_writer.c:write_elim_usage_proof

**/
typedef struct rete_net_state_t {
  const char* proof_branch_id;
  unsigned int step_no;
  substitution_list ** subs;
  sub_alpha_queue ** sub_alpha_queues;
  sub_alpha_queue ** sub_alpha_queue_roots;
  bool * is_applied_fact;
  const rete_net* net;
  unsigned int * global_step_counter;
  unsigned int cursteps;
  unsigned int start_step_no;
  fresh_const_counter fresh;
  fact_set* facts;
  fact_set* old_fact_set;
  const term** domain;
  size_t size_domain;
  unsigned int n_domain;
  const char** constants;
  size_t size_constants;
  unsigned int n_constants;
  bool verbose;
  rule_instance* end_of_branch;
  rule_instance_stack* elim_stack;
  struct rete_net_state_t ** branches;
  rule_queue* axiom_inst_queue[];
} rete_net_state;

#endif
