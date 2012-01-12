/* proof_branch.h

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
#ifndef __INCLUDED_PROOF_BRANCH_H
#define __INCLUDED_PROOF_BRANCH_H

#include "theory.h"
#include "rule_instance_stack.h"
/**
   Represents a branch in the proof tree. 

   Used by rete_state_single, (In rete_state the state has this information)

   The component array children has size 0 or rule->rhs->n_args, since this is known at the point when the children are added at all
**/
typedef struct proof_branch_t {
  unsigned int id;
  char* name;
  timestamp start_step;
  rule_instance_stack* elim_stack;
  timestamp end_step;
  struct proof_branch_t * parent;
  struct proof_branch_t ** children;
  unsigned int n_children;
  unsigned int size_children;
} proof_branch;

proof_branch* create_root_proof_branch(void);
void delete_proof_branch_tree(proof_branch*);
void end_proof_branch(proof_branch*, timestamp, unsigned int);
proof_branch* create_child_branch(proof_branch*, const theory*);
void prune_proof(proof_branch* parent, unsigned int);
#endif
