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
#ifndef __INCLUDED_RETE_STATE_SINGLE
#define __INCLUDED_RETE_STATE_SINGLE

#include "rete_net.h"
#include "substitution.h"
#include "substitution_state_store.h"


/**
   This version of a rete state is intended for a prover without or-parallellism, but
   with parallellism in the insertion into the rule queues
**/
typedef struct rete_net_state_single_t {
  substitution_state_store * subs;
  rule_instance_store * rule_queue;
  const rete_net* net;
  fresh_const_counter fresh;  
  const char** constants;
  size_t size_constants;
  unsigned int n_constants;
  bool verbose;
  rule_instance_stack* elim_stack;
  fact_set ** factset;
  bool finished;
} rete_net_state;




#endif
