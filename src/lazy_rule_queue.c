/* lazy_rule_queue.c

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

/*
  Implements functionality for the lazy version of RETE. 
  That is, querying whether
  there is a rule in the queue for an axiom, or if not, whether
  there is a substitution waiting to be inserted into some alpha node.

  Most of these functions are called from functions in strategy.c
*/

#include "common.h"
#include "rete.h"
#include "lazy_rule_queue.h"

bool axiom_has_new_instance(size_t axiom_no, rete_net_state * state){
  sub_alpha_queue * sub_list = state->sub_alpha_queues[axiom_no];
  if(state->axiom_inst_queue[axiom_no]->n_queue > 0)
    return true;

  while(sub_list != NULL){
    insert_rete_alpha_fact_children(state, sub_list->alpha_node, sub_list->fact, sub_list->sub, true);

    assert(sub_list->prev == NULL);

    state->sub_alpha_queues[axiom_no] = sub_list->next;
    free(sub_list);
    sub_list = state->sub_alpha_queues[axiom_no];

    if(state->axiom_inst_queue[axiom_no]->n_queue > 0)
      return true;
  }
  return false;
}

