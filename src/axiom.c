/* axiom.c

   Copyright 2011 ?

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

/**
   Functions used by the geolog_parser to create the rules data structures 

**/
#include "common.h"
#include "disjunction.h"
#include "conjunction.h"
#include "axiom.h"
#include "fresh_constants.h"
#include "rete.h"


/**
   Axiom represents a "rule", a part of the theory
   Constructors and destructor
**/

axiom* create_axiom(conjunction* lhs, disjunction* rhs){
  int i;
  axiom * ret_val = malloc_tester(sizeof(axiom));
  ret_val->type = normal;
  ret_val->lhs = lhs;
  ret_val->rhs = rhs;
  ret_val->is_existential = false;
  ret_val->has_domain_pred = false;
  ret_val->exist_vars = init_freevars();
  reset_freevars(lhs->bound_vars);
  for(i = 0; i < rhs->n_args; i++){
    remove_freevars(rhs->args[i]->bound_vars, lhs->free_vars);
    remove_freevars(rhs->args[i]->free_vars, rhs->args[i]->bound_vars);
    rhs->free_vars = plus_freevars(rhs->free_vars, rhs->args[i]->free_vars);
    if(rhs->args[i]->bound_vars->n_vars > 0){
      ret_val->is_existential = true;
      ret_val->exist_vars = plus_freevars(ret_val->exist_vars, rhs->args[i]->bound_vars);
    }
    if(rhs->args[i]->has_domain_pred)
      ret_val->has_domain_pred = true;
  }
  return ret_val;
}

axiom* create_fact(disjunction *rhs){
  unsigned int i;
  axiom * ret_val = malloc_tester(sizeof(axiom));
  ret_val->type = fact;
  ret_val->lhs = create_empty_conjunction();
  ret_val->rhs = rhs;
  ret_val->exist_vars = init_freevars();
  ret_val->is_existential = false;
  reset_freevars(rhs->free_vars);
  for(i = 0; i < rhs->n_args; i++){
    reset_freevars(rhs->args[i]->free_vars);
    if(rhs->args[i]->bound_vars->n_vars > 0){
      ret_val->is_existential = true;
      ret_val->exist_vars = plus_freevars(ret_val->exist_vars, rhs->args[i]->bound_vars);
    }	  
  }
  return ret_val;
}


axiom* create_goal(conjunction* lhs){
  axiom * ret_val = malloc_tester(sizeof(axiom));
  ret_val->type = goal;
  ret_val->lhs = lhs;
  ret_val->rhs = create_empty_disjunction();
  ret_val->is_existential = false;
  ret_val->exist_vars = init_freevars();
  reset_freevars(lhs->bound_vars);
  return ret_val;
}

void delete_axiom(axiom* a){
  if(a->type == fact)
    free((conjunction*) a->lhs);
  else
    delete_conjunction((conjunction*) a->lhs);
  delete_disjunction((disjunction*) a->rhs);
  del_freevars(a->exist_vars);
  free(a);
}

/**
   Creating the rete nodes for the axiom, except if it is a fact. 
   Facts are just added to the fact queue at the beginning

   The rule_freevars is created and not deleted, as it is passed on to the rete nodes

   axiom_no is a unique number assigned to each axiom. This is passed on to the rete nodes, 
   and is used in the lazy RETE version.

   This is the only place where net->lazy is used, and it leads to setting the
   propagate value of alpha nodes in the left hand side to the opposite value
**/
void create_rete_axiom_node(rete_net* net, const axiom* ax, size_t axiom_no){
  rete_node* node;
  if(ax->type != fact){
    const freevars* rule_free_vars;
    if(ax->type == normal) {
      rule_free_vars = ax->rhs->free_vars;
    } else {   // not normal 
      assert(ax->type == goal);
      rule_free_vars = init_freevars();
    }
    node = create_rete_conj_node(net, 
				 ax->lhs, 
				 rule_free_vars,
				 !(net->lazy),
				 axiom_no);
    if(ax->type == normal) {
      node = create_rete_disj_node(net, 
				   node, 
				   ax->rhs,
				   axiom_no);
    }
    create_rule_node(net, node, ax, rule_free_vars, axiom_no);
  }  
}
    
void set_axiom_name(axiom* a, const char* name){
  a->name = name;
}
    

/**
   The  free variables 
**/
freevars* free_axiom_variables(const axiom* axm, freevars* vars){
  return free_conj_variables(axm->lhs, vars);
}

/**
   For testing of the axiom basic functionality and sanity
**/
bool test_axiom(const axiom* a, size_t no){
  bool found_exist = false;
  unsigned int i;
  freevars* fv;
  fv = init_freevars();
  assert(a->axiom_no == no);
  switch(a->type){
  case goal:
    assert(a->rhs->n_args == 0);
    test_conjunction(a->lhs);
    fv = free_conj_variables(a->lhs, fv);
    break;
  case fact:
    assert(a->lhs->n_args == 0);
    test_disjunction(a->rhs);
    break;
  case normal:
    test_conjunction(a->lhs);
    test_disjunction(a->rhs);
    fv = free_conj_variables(a->lhs, fv);
    break;
  default:
    assert(false);
  }
  for(i = 0; i < a->rhs->n_args; i++){
    if(a->rhs->args[i]->bound_vars->n_vars > 0){
      assert(empty_intersection(fv, a->rhs->args[i]->bound_vars));
      found_exist = true;
    }
  }
  assert(found_exist == a->is_existential);  
  return true;
}


/**
   Pretty printing in standard fol format
**/
void print_fol_axiom(const axiom * ax, FILE* stream){
  if(ax->type != fact){
    print_fol_conj(ax->lhs, stream);
    if(ax->lhs->n_args > 0)
      fprintf(stream, " -> ");
  }
  if(ax->type == goal)
    fprintf(stream, "goal");
  else
    print_fol_disj(ax->rhs, stream);
  fprintf(stream, " ");
}

/**
   Pretty printing in dot (graphviz) format
**/
void print_dot_axiom(const axiom * ax, FILE* stream){
  if(ax->type != fact){
    print_dot_conj(ax->lhs, stream);
    if(ax->lhs->n_args > 0)
      fprintf(stream, " ⇒ ");
  }
  if(ax->type == goal)
    fprintf(stream, "goal");
  else
    print_dot_disj(ax->rhs, stream);
  fprintf(stream, " ");
}


/**
   Pretty printing in geolog input format
**/
void print_geolog_axiom(const axiom* a, FILE* stream){
  if(a->type == fact)
    fprintf(stream, "true");
  else 
    print_geolog_conj(a->lhs, stream);
  fprintf(stream,"=>");
  if(a->type==goal)
    fprintf(stream, "goal");
  else
    print_geolog_disj(a->rhs, stream);
  fprintf(stream, " .");
}

void print_coq_axiom(const axiom* a, FILE* stream){
  fprintf(stream, "Hypothesis %s : ", a->name);
    freevars* f = a->lhs->bound_vars;
  if(f->n_vars > 0){
    fprintf(stream, "forall ");
    for(i = 0; i < f->n_vars; i++)
      fprintf(stream, "%s ", f->vars[i]->name);
    fprintf(stream, ": dom,\n");
  }
  if(a->type != fact){
    print_coq_conjunction(a->lhs, stream);
    fprintf(stream, " -> ");
  }
  print_coq_disjunction(a->rhs, stream);
  fprintf(".\n");
}
      

/**
   Returns some properties of the axioms/rules
   A definite axiom/rule has no disjunction / splitting in the consequents/rhs
   An existential axiom/rule has at least one disjunct in the consequent with existentially bound variable(s)
**/
bool is_definite(axiom* a){
  return(a->rhs->n_args <= 1);
}
bool is_existential(axiom* a){
  return(a->is_existential);
}
