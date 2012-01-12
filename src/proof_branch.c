/* proof_branch.c

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

#include "common.h"
#include "proof_branch.h"
#include <stdio.h>

/**
   Only called from create_rete_state_single
**/
proof_branch* create_root_proof_branch(void){
  proof_branch* br = malloc_tester(sizeof(proof_branch));
  br->id = 0;
  br->name = "";
  br->start_step = create_normal_timestamp(0);
  br->n_children = 0;
  br->size_children = 0;
  br->parent = NULL;
  br->elim_stack = initialize_ri_stack();
  return br;
}
/**
   Only called from destroy_rete_state_single
**/
void delete_proof_branch(proof_branch* br){
  delete_ri_stack(br->elim_stack);
  free(br);
}

/**
   Only called from destroy_rete_state_single
**/
void delete_proof_branch_tree(proof_branch* br){
  unsigned int i;
  for(i = 0; i < br->n_children; i++)
    delete_proof_branch_tree(br->children[i]);
   if(br->n_children > 0)
    free(br->children);
  delete_proof_branch(br);
}
/**
   Called from prover_single when reaching a goal/leaf or
   a disjunction

   Must be called (correctly) before create_child_branch
**/
void end_proof_branch(proof_branch* br, timestamp step, unsigned int n_children){
  br->end_step = step;
  br->size_children = n_children;
  if(br->size_children > 0)
    br->children = calloc_tester(br->size_children, sizeof(proof_branch*));

}

/**
   Called during treatment of disjunctions

   Assumes that end_proof_branch has been called first, 
   with the right number of children
**/
proof_branch* create_child_branch(proof_branch* br, const theory* th){
  proof_branch* child = malloc_tester(sizeof(proof_branch));
  assert(br->n_children < br->size_children);
  br->children[br->n_children] = child;
  child->start_step = br->end_step;
  child->n_children = 0;
  child->name = malloc_tester(strlen(br->name) + 20);
  sprintf(child->name, "%s_%i", br->name, br->n_children);
  child->elim_stack = initialize_ri_stack();
  child->parent = br;
  br->n_children++;
  return child;
}

/** 
    Called when prover_single sees that a disjunction 
    was unnecessary
**/
void prune_proof(proof_branch* parent, unsigned int child_no){
  unsigned int i;
  proof_branch* child = parent->children[child_no];
  assert(child->parent == parent);
  assert(child_no < parent->n_children);
  for(i = 0; i < parent->n_children; i++){
    if(i != child_no)
      delete_proof_branch_tree(parent->children[i]);
  }
  free(parent->children);
  add_ri_stack(parent->elim_stack, child->elim_stack);
  parent->end_step = child->end_step;
  parent->n_children = child->n_children;
  parent->children = child->children;
  delete_proof_branch(child);
}
  

