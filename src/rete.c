/* rete.c

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
#include "predicate.h"
#include "fresh_constants.h"
#include "rete.h"
#include "theory.h"


/** 
    Constructors
**/


void add_rete_child(rete_node* parent, const rete_node* new_child){
  parent->n_children ++;
  assert(parent->size_children > 0);
  if(parent->n_children >= parent->size_children){
    parent->size_children *= 2;
    parent->children = realloc(parent->children, (parent->size_children + 1) * sizeof(rete_node*));
  }
  parent->children[parent->n_children-1] = new_child;
  return;
}

void init_rete_node(rete_node* n, enum rete_node_type type, rete_node* left_parent, const freevars* free_vars){
  n->type = type;
  n->size_children = 10;
  n->children = calloc_tester(n->size_children, sizeof(rete_node*));
  n->n_children = 0;
  n->left_parent = left_parent;
  n->free_vars = free_vars;
  if(left_parent != NULL){
    assert(left_parent->size_children > 0);
    add_rete_child(left_parent, n);
  }
}

rete_node* _create_rete_node(enum rete_node_type type, rete_node* left_parent, const freevars* free_vars){
  rete_node* bn = malloc_tester(sizeof(rete_node));  
  init_rete_node(bn, type, left_parent, free_vars);
  return bn;
}

void init_selector_node(predicate* p, rete_node* n){
  init_rete_node(n, selector, NULL, init_freevars());
  n->val.selector = p;
  return;
}

/**
   This is used during construction of the rete net
**/
rete_node* get_selector(size_t pred_no, rete_net* net){
  rete_node* retval = & net->selectors[pred_no];
  assert(retval->val.selector == net->th->predicates[pred_no]);
  return retval;
}

/**
   This is used by the prover proper
**/
const rete_node* get_const_selector(size_t pred_no, const rete_net* net){
  const rete_node* retval = & net->selectors[pred_no];
  assert(retval->val.selector == net->th->predicates[pred_no]);
  return retval;
}

/**
   Initializes rete net

   Note that is is safe to have selectors as an array of structures, since
   the array is never realloced
**/
rete_net* init_rete(const theory* th, unsigned long maxsteps){
  unsigned int i;
  rete_net* net = malloc_tester(sizeof(rete_net) + th->n_predicates * sizeof(rete_node));
  net->n_selectors = th->n_predicates;
  net->th = th;
  net->maxsteps = maxsteps;

  net->n_subs = 0;
  for(i = 0; i < net->n_selectors; i++)
    init_selector_node(th->predicates[i], &net->selectors[i]);
  return net;
}

/*
rete_node* create_store_node(rete_net* net, rete_node* parent, const freevars* vars){
  rete_node* node = _create_rete_node(store, parent, vars);
  node->val.beta.b_store_no = net->n_subs++;
  return node;
}
*/
rete_node* create_beta_left_root(void){
  return _create_rete_node(beta_root, NULL, NULL);
}


/**
   Constructs an alpha node in a rete network. 
   There is one alpha node for each atom (a predicate applied to some terms) 
   in the left hand side.
   There is also "negative" alpha nodes for the atoms on the right hand sides, 
   used to inhibit construction of already used rule instances.

   propagate set to true is the standard "non-lazy" approach
**/

rete_node* create_alpha_node(rete_node* left_parent, unsigned int arg_no, const term* val, const freevars* free_vars, bool propagate){
  rete_node* node = _create_rete_node(alpha, left_parent, free_vars);
  node->val.alpha.argument_no = arg_no;
  node->val.alpha.value = val;
  node->val.alpha.propagate = propagate;
  return node;
}


rete_node* _create_beta_node(rete_net* net, rete_node* left_parent, rete_node* right_parent, enum rete_node_type type, const freevars* free_vars){
  rete_node* node = _create_rete_node(type, left_parent, free_vars);
  assert(left_parent->n_children > 0);
  node->val.beta.right_parent = right_parent;
  add_rete_child(right_parent, node);
  assert(right_parent->n_children > 0);

  node->val.beta.b_store_no = net->n_subs++;
  node->val.beta.a_store_used_no = net->n_subs++;
  node->val.beta.a_store_no = net->n_subs++;
  assert(node->val.beta.b_store_no == net->n_subs-3);
  return node;
}


rete_node* create_beta_and_node(rete_net* net, rete_node* left_parent, rete_node* right_parent, const freevars* free_vars){
  assert(left_parent->type == beta_and || left_parent->type == beta_not || left_parent->type == beta_root || left_parent->type == beta_or);
  assert(right_parent->type == alpha || right_parent->type == beta_not  || left_parent->type == beta_or || right_parent->type == beta_and || right_parent->type == selector);
  return  _create_beta_node(net, left_parent, right_parent, beta_and, free_vars);
}

rete_node* create_beta_or_node(rete_net* net, rete_node* left_parent, rete_node* right_parent, const freevars* free_vars){
  assert(left_parent->type == beta_and || left_parent->type == beta_not || left_parent->type == beta_root || left_parent->type == beta_or);
  assert(right_parent->type == alpha || right_parent->type == beta_not || right_parent->type == beta_and || left_parent->type == beta_or || right_parent->type == selector);
  return  _create_beta_node(net, left_parent, right_parent, beta_or, free_vars);
}

rete_node* create_beta_not_node(rete_net* net, rete_node* left_parent, rete_node* right_parent, const freevars* free_vars){
  assert(left_parent->type == beta_and  ||left_parent->type == beta_not   || left_parent->type == beta_or ||  left_parent->type == alpha);
  assert(right_parent->type == alpha || right_parent->type == beta_and || left_parent->type == beta_or);
  return  _create_beta_node(net, left_parent, right_parent, beta_not, free_vars);
}

void create_rule_node(rete_net* net, rete_node* parent, const axiom* ax, const freevars* vars){
  rete_node* node = _create_rete_node(rule, parent, vars);
  assert(parent->n_children > 0);
  node->val.rule.store_no = net->n_subs++;
  node->val.rule.axm = ax;
}

/**
   Destructors
**/

/**
   The pointer from the parent must be removed to prevent 
   duplicate traversal of the tree
**/
void remove_parent_pointer(rete_node* node, rete_node* parent){
  unsigned int i;
  for(i =  0; i < parent->n_children; i++)
    if(parent->children[i] == node){
      parent->children[i] = NULL;
      return;
    }
  assert(false);
}


void delete_rete_node(rete_node* node){
  unsigned int i;
  if(node->left_parent != NULL)
    remove_parent_pointer(node, (rete_node*) node->left_parent);
  else 
    assert(node->type == selector || node->type == beta_root);
  if(node->type == beta_and || node->type == beta_not)
    remove_parent_pointer(node, (rete_node*) node->val.beta.right_parent);
  for(i = 0; i < node->n_children; i++){
    rete_node* child = (rete_node*) node->children[i];
    if(child != NULL){
      assert(node->size_children > 0);
      delete_rete_node((rete_node*) node->children[i]);
      free((rete_node*) node->children[i]);
    }
  }
  free(node->children);
}

void delete_rete_net(rete_net* net){
  unsigned int i;
  for(i = 0; i < net->n_selectors; i++)
    delete_rete_node((rete_node*) & net->selectors[i]);
  free(net);
}


/**
   Testing a rete network
**/

void test_rete_node(const rete_node* node, void (*child_test)(const rete_node*)){
  unsigned int i;
  assert(node->type == selector || node->type == alpha || node->type == beta_root || node->type == beta_and || node->type == beta_not || node->type == rule);
  assert(node->size_children > 0);
  assert(node->n_children < node->size_children);
  assert(node->n_children > 0 || node->type == rule || node->type == selector);
  for(i = 0; i < node->n_children; i++){
    const rete_node* child = node->children[i];
    assert(child != NULL);
    child_test(child);
  }
}

void test_rete_beta(const rete_node* node){
  assert(node->type != alpha && node->type != selector);
  assert(node->type == rule || node->n_children == 1);
  test_rete_node(node, test_rete_beta);
}


void test_rete_alpha(const rete_node* node){
  assert(node->type == alpha || node->type == beta_and || node->type == beta_not);
  if(node->type == alpha){
    test_rete_node(node, test_rete_alpha);
  } else
    test_rete_node(node, test_rete_beta);
}

void test_rete_selector(const rete_node* sel){
  assert(sel->type == selector);
  test_rete_node(sel, test_rete_alpha);
}

bool test_rete_net(const rete_net* net){
  unsigned int i;
  assert(net->n_selectors == net->th->n_predicates);
  assert(net->n_selectors > 0);
  for(i = 0; i < net->n_selectors; i++)
    test_rete_selector(&net->selectors[i]);
  return true;
}


/**
   Printing out a rete network
**/
void print_rete_node(const rete_node*, FILE*, unsigned int);

void print_selector(const rete_node* sel, FILE* stream){
  assert(sel->type == selector);
  fprintf(stream, "%s(%zi): %i occurrences", sel->val.selector->name, sel->val.selector->arity, sel->n_children);
  print_rete_node(sel, stream,1);
}

void print_rete_alpha_node(const rete_node* node, FILE* stream){
  assert(node->type == alpha);
  fprintf(stream, ": argument #%i unifies with ", node->val.alpha.argument_no + 1);
  print_fol_term(node->val.alpha.value, stream);
  fprintf(stream, ".");
}

void print_dot_alpha_node(const rete_node* node, FILE* stream){
  assert(node->type == alpha);
  fprintf(stream, ": argument #%i unifies with", node->val.alpha.argument_no + 1);
  print_fol_term(node->val.alpha.value, stream);
  fprintf(stream, ".");
}

const char* get_rete_node_type_name(const rete_node* node){
  switch(node->type)
    {
    case selector:
      return "selector";
      break;
    case alpha:
      return "alpha";
      break;
    case beta_and:
      return "beta_and";
      break;
    case beta_or:
      return "beta_or";
      break;
    case beta_not:
      return "beta_not";
      break;
 case beta_root:
      return "beta_root";
      break;
 case rule:
      return  "rule";
      break;
    default:
      fprintf(stderr, "unknown rete node type: %i", node->type);
      assert(false);
      return "unknown??";
    }
}

void print_rete_node_type(const rete_node* node, FILE* stream){
  fprintf(stream, "%s", get_rete_node_type_name(node));
  switch(node->type)
    {
    case selector:
      fprintf(stream, ": %s(%zi)", node->val.selector->name, node->val.selector->arity);
      break;
    case alpha:
      print_rete_alpha_node(node, stream);
      break;
    case beta_and:
      break;
    case beta_not:
      break;
 case beta_root:
      break;
 case rule:
      fprintf(stream, ": ");
      print_fol_axiom(node->val.rule.axm, stream);
      break;
    default:
      fprintf(stream, "unknown rete node type: %i", node->type);
      assert(false);
    }
  return;
}
   
void print_rete_node(const rete_node* node, FILE* stream, unsigned int indent){
  unsigned int i;
  for(i = 0; i < indent; i++)
    fprintf(stream, "  ");
  if(node->type != selector){
    print_rete_node_type(node, stream);
    if(node->n_children == 1)
      fprintf(stream, " 1 child:");
    else if(node->n_children > 1)
      fprintf(stream, " %i children:", node->n_children);
    fprintf(stream, "\n");
  }
  for(i = 0; i < node->n_children; i++){
    assert(node->children[i]->type != selector);
    print_rete_node(node->children[i], stream, indent+1);
  }
}

void print_dot_rete_node(const rete_node* node, FILE* stream){
  unsigned int i;
  fprintf(stream, "n%li [label=\"", (unsigned long) node);
  if(node->type == rule)
    print_dot_axiom(node->val.rule.axm, stream);
  else
    print_rete_node_type(node, stream);
  fprintf(stream, "\"]\n");
  for(i = 0; i < node->n_children; i++){
    assert(node->children[i]->type != selector);
    fprintf(stream, "n%li -> n%li\n", (unsigned long) node, (unsigned long) node->children[i]);
    print_dot_rete_node(node->children[i], stream);
  }
}



void print_rete_net(const rete_net* net, FILE* stream){
  unsigned int i;
  fprintf(stream,"%zi predicates: ", net->n_selectors);
  for(i = 0; i < net->n_selectors; i++){
    fprintf(stream, "%s(%zi)", net->selectors[i].val.selector->name, net->selectors[i].val.selector->arity);
    if(i+1 < net->n_selectors)
      fprintf(stream,", ");
  }
  fprintf(stream, "\n");
  for(i = 0; i < net->n_selectors; i++)
    print_selector(& net->selectors[i], stream);
}


void print_dot_rete_net(const rete_net* net, FILE* stream){
  unsigned int i;
  fprintf(stream, "strict digraph RETE {\n");
  for(i = 0; i < net->n_selectors; i++)
    print_dot_rete_node(& net->selectors[i], stream);
  fprintf(stream, "}\n");
}

