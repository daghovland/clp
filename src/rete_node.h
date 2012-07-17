/* rete_node.h

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
#ifndef __INCLUDED_RETE_NODE
#define __INCLUDED_RETE_NODE

#include "predicate.h"
#include "term.h"
#include "atom.h"
#include "conjunction.h"
#include "disjunction.h"
#include "axiom.h"
#include "theory.h"
#include "substitution.h"
#include "rule_queue.h"
#include "fresh_constants.h"
#include "fact_set.h"
#include "strategy.h"

/**
   There is one predicate node for each predicate name occurring in the theory

   alpha nodes test intravariable constraints
   value nodes test that a variable has a specific term value
   bottom are special nodes with beta_node children. 
   All other nodes have only alpha_node children

 beta nodes test that different atoms have the same values
 where they are supposed to 

 beta_and and beta_not nodes have a right-parent (two-input-nodes)
 
 beta_or nodes are a new, experimental, not finished node type. 
 Used to possibly support arbitraritly complex formulas (but without negation) on the left hand side
 
 alpha nodes test argument number being unifiable with the term at that position

 a_store_no, b_store_no are indexes into the subsititution_list array "subs" in the rete_state.
 They represent the caches/stores of already treated substitutions in the alpha and beta node, respectively.
 
 a_store_used_no is also an index into subs, 
 but points to the last used substitution_list entry for the alpha store.
 The latter is used by the "lazy" extension of rete.

 The value propagate is true if an alpha node should pass on all inserted substitutions. 
 This is the standard "non-lazy" approach, and should also be the approach in the alpha nodes 
 above a beta_not node. 
 Propagate can be false in alpha nodes not above beta_not. Then inserted values will not be 
 automatically passed on, but rather put in a queue on the rule node, which then will
 ask for it when more rule instances are needed.
**/
enum rete_node_type { alpha, beta_and, beta_root, beta_not, beta_or, equality_node, selector_node, rule_node };

struct rete_node_t {
  enum rete_node_type type;
  union {
    clp_predicate * selector;
    struct alpha_t {
      const clp_term* value;
      unsigned int argument_no;
      bool propagate;
    } alpha;
    struct beta_t {
      const struct rete_node_t* right_parent;
      unsigned int a_store_no;
      unsigned int a_store_used_no;
      unsigned int b_store_no;
    } beta;
    struct equality_t {
      const clp_term* t1;
      const clp_term* t2;
      unsigned int b_store_no;
    } equality;
    struct rule_t {
      const clp_axiom* axm;
      unsigned int store_no;
    } rule;
  } val;
  const freevars* free_vars;
  const struct rete_node_t** children;
  int n_children;
  bool in_positive_lhs_part;
  size_t size_children;
  const struct rete_node_t* left_parent;
  unsigned int rule_no;
};

typedef struct rete_node_t rete_node;

#endif
