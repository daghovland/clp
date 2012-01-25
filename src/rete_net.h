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
#ifndef __INCLUDED_RETE_NET_H
#define __INCLUDED_RETE_NET_H

#include "common.h"
#include "theory.h"
#include "rete_node.h"
#include "strategy.h"

#ifdef HAVE_PTHREAD
#include <pthread.h>
#endif



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

   sub_mutexes is of length n_subs. There is a mutex for each of the substitution list structures, since
   these must be protected when they are iterated. This is because the thread-safe version of sub_list_iter
   has too high overhead (3x run-time)

   n_rules is the number of rules entered into the rete net. 
   This is set in create_rete_net in theory.c, usually to the number of axioms that are not facts.
**/
typedef struct rete_net_t {
  unsigned int n_subs;
  unsigned int n_selectors;
  rete_node ** rule_nodes;
  const theory* th;
  bool existdom;
  bool lazy;
  bool coq;
  bool treat_all_disjuncts;
  bool factset_lhs;
  bool use_beta_not;
  bool multithread_rete;
  strategy strat;
#ifdef HAVE_PTHREAD
  pthread_mutex_t * sub_mutexes;
#endif
  //unsigned int size_history;
  // rule_instance ** history;
  unsigned long maxsteps;
  rete_node selectors[];
} rete_net;



#endif
