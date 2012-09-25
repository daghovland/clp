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
#include "theory.h"


/**
   Axiom represents a "rule", a part of the theory
   Constructors and destructor
**/

clp_axiom* create_axiom(clp_conjunction* lhs, clp_disjunction* rhs, theory* th){
  int i;
  clp_axiom * ret_val = malloc_tester(sizeof(clp_axiom));
  ret_val->type = normal;
  ret_val->name = NULL;
  ret_val->has_name = false;
  ret_val->lhs = lhs;
  ret_val->rhs = rhs;
  ret_val->is_existential = false;
  ret_val->exist_vars = init_freevars();
  reset_freevars(lhs->bound_vars);
  for(i = 0; i < rhs->n_args; i++){
    remove_freevars(rhs->args[i]->bound_vars, lhs->free_vars);
    remove_freevars(rhs->args[i]->free_vars, rhs->args[i]->bound_vars);
    rhs->args[i]->is_existential = rhs->args[i]->bound_vars->n_vars > 0;
    rhs->free_vars = plus_freevars(rhs->free_vars, rhs->args[i]->free_vars);
    if(rhs->args[i]->bound_vars->n_vars > 0){
      ret_val->is_existential = true;
      ret_val->exist_vars = plus_freevars(ret_val->exist_vars, rhs->args[i]->bound_vars);
    }
  }
  fix_equality_vars(lhs, th);
  return ret_val;
}

clp_axiom* create_fact(clp_disjunction *rhs, theory* th){
  unsigned int i;
  clp_axiom * ret_val = malloc_tester(sizeof(clp_axiom));
  ret_val->type = fact;
  ret_val->name = NULL;
  ret_val->has_name = false;
  ret_val->lhs = create_empty_conjunction(th);
  ret_val->rhs = rhs;
  ret_val->exist_vars = init_freevars();
  ret_val->is_existential = false;
  reset_freevars(rhs->free_vars);
  for(i = 0; i < rhs->n_args; i++){
    reset_freevars(rhs->args[i]->free_vars);
    if(rhs->args[i]->bound_vars->n_vars > 0){
      rhs->args[i]->is_existential = true;
      ret_val->is_existential = true;
      ret_val->exist_vars = plus_freevars(ret_val->exist_vars, rhs->args[i]->bound_vars);
    } else 
      rhs->args[i]->is_existential = false;
  }
  return ret_val;
}


clp_axiom* create_goal(clp_conjunction* lhs){
  clp_axiom * ret_val = malloc_tester(sizeof(clp_axiom));
  ret_val->type = goal;
  ret_val->lhs = lhs;
  ret_val->rhs = create_empty_disjunction();
  ret_val->is_existential = false;
  ret_val->exist_vars = init_freevars();
  reset_freevars(lhs->bound_vars);
  return ret_val;
}

void delete_axiom(clp_axiom* a){
  if(a->type == fact)
    free((clp_conjunction*) a->lhs);
  else
    delete_conjunction((clp_conjunction*) a->lhs);
  delete_disjunction((clp_disjunction*) a->rhs);
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
rete_node* create_rete_axiom_node(rete_net* net, const clp_axiom* ax, unsigned int axiom_no, bool use_beta_not){
  rete_node* node;
  const freevars* rule_free_vars;
  if(ax->type != goal) {
    rule_free_vars = ax->rhs->free_vars;
  } else {   // not normal 
    assert(ax->type == goal);
    rule_free_vars = init_freevars();
  }
  node = create_rete_conj_node(net, 
			       ax->lhs, 
			       rule_free_vars,
			       !(net->lazy),
			       true,
			       axiom_no);
  if(ax->type != goal && use_beta_not && ax->lhs->n_args > 2) {
    node  = insert_beta_not_nodes(net, 
				  ax->lhs, 
				  ax->rhs, 
				  node, 
				  axiom_no);
  }
  return create_rule_node(net, node, ax, rule_free_vars, axiom_no);
}
    
/**
   The axiom name must be given a prefix, to remove the overlap between predicate names and axiom names
   that exist in some theories
**/
void set_axiom_name(clp_axiom* a, const char* name){
  a->name = malloc(strlen(name) + 5);
  a->has_name = true;
  sprintf((char *) a->name, "H_%s", name);
}
    
bool axiom_has_name(const clp_axiom* a){
  return a->has_name;
}

/**
   The  free variables 
**/
freevars* free_axiom_variables(const clp_axiom* axm, freevars* vars){
  return free_conj_variables(axm->lhs, vars);
}

/**
   For testing of the axiom basic functionality and sanity
**/
bool test_axiom(const clp_axiom* a, size_t no, const constants* cs){
#ifndef NDEBUG
  bool found_exist = false;
#endif
  unsigned int i;
  freevars* fv;
  fv = init_freevars();
  assert(a->axiom_no == no);
  switch(a->type){
  case goal:
    assert(a->rhs->n_args == 0);
    test_conjunction(a->lhs, cs);
    fv = free_conj_variables(a->lhs, fv);
    break;
  case fact:
    assert(a->lhs->n_args == 1);
    test_disjunction(a->rhs, cs);
    break;
  case normal:
    test_conjunction(a->lhs, cs);
    test_disjunction(a->rhs, cs);
    fv = free_conj_variables(a->lhs, fv);
    break;
  default:
    assert(false);
  }
  for(i = 0; i < a->rhs->n_args; i++){
    if(a->rhs->args[i]->bound_vars->n_vars > 0){
      assert(empty_intersection(fv, a->rhs->args[i]->bound_vars));
#ifndef NDEBUG
      found_exist = true;
#endif
    }
  }
  assert(found_exist == a->is_existential);  
  return true;
}


/**
   Pretty printing in standard fol format
**/
void print_fol_axiom(const clp_axiom * ax, const constants* cs, FILE* stream){
  if(ax->type != fact){
    print_fol_conj(ax->lhs, cs, stream);
    if(ax->lhs->n_args > 0)
      fprintf(stream, " -> ");
  }
  if(ax->type == goal)
    fprintf(stream, "goal");
  else
    print_fol_disj(ax->rhs, cs, stream);
  fprintf(stream, " ");
}

/**
   Pretty printing in dot (graphviz) format
**/
void print_dot_axiom(const clp_axiom * ax, const constants* cs, FILE* stream){
  if(ax->type != fact){
    print_dot_conj(ax->lhs, cs, stream);
    if(ax->lhs->n_args > 0)
      fprintf(stream, " ⇒ ");
  }
  if(ax->type == goal)
    fprintf(stream, "goal");
  else
    print_dot_disj(ax->rhs, cs, stream);
  fprintf(stream, " ");
}


/**
   Pretty printing in geolog input format
**/
void print_geolog_axiom(const clp_axiom* a, const constants* cs, FILE* stream){
  if(a->type == fact)
    fprintf(stream, "true");
  else 
    print_geolog_conj(a->lhs, cs,  stream);
  fprintf(stream," => ");
  if(a->type==goal)
    fprintf(stream, "goal");
  else
    print_geolog_disj(a->rhs, cs, stream);
  fprintf(stream, ".");
}

/**
   Pretty printing in CL.pl format
**/
void print_clpl_axiom(const clp_axiom* a, const constants* cs, FILE* stream){
  if(a->type == fact)
    fprintf(stream, "true");
  else 
    print_clpl_conj(a->lhs, cs,  stream);
  fprintf(stream,"=>");
  if(a->type==goal)
    fprintf(stream, "goal");
  else
    print_clpl_disj(a->rhs, cs, stream);
  fprintf(stream, " .");
}

void print_coq_axiom(const clp_axiom* a, const constants* cs, FILE* stream){
  unsigned int i;
  assert(a->has_name);
  fprintf(stream, "Hypothesis %s : ", a->name);
  freevars* f = a->lhs->free_vars;
  if(f->n_vars > 0){
    fprintf(stream, "forall ");
    for(i = 0; i < f->n_vars; i++)
      fprintf(stream, "%s ", f->vars[i]->name);
    fprintf(stream, ": %s,\n", DOMAIN_SET_NAME);
  }
  if(a->type != fact){
    if(print_coq_conj(a->lhs, cs, stream))
      fprintf(stream, " -> ");
  }
  if(a->rhs->n_args == 0)
    fprintf(stream, "goal");
  else
    print_coq_disj(a->rhs, cs, stream);
  fprintf(stream, ".\n");
}

void print_tptp_axiom(const clp_axiom* a, const constants* cs, FILE *stream){
  freevars* f = a->lhs->free_vars;
  unsigned int i;
  fprintf(stream, "fof(%s,axiom, ", a->name);
  if(f->n_vars > 0){
    fprintf(stream, "![ ");
    for(i = 0; i < f->n_vars; i++){
      fprintf(stream, "%s", f->vars[i]->name);
      if(i +1 < f->n_vars)
	fprintf(stream, ", ");
    }
    fprintf(stream, "] : ");
  }
  fprintf(stream, "(");
  if(a->type != fact)
    print_tptp_conj(a->lhs, cs, stream);
  else
    fprintf(stream, " $true "); 
  fprintf(stream, " => ");
  if(a->rhs->n_args == 0)
    fprintf(stream, " goal");
  else
    print_tptp_disj(a->rhs, cs, stream);
  fprintf(stream, " )).");
}      

/**
   Returns some properties of the axioms/rules
   A definite axiom/rule has no disjunction / splitting in the consequents/rhs
   An existential axiom/rule has at least one disjunct in the consequent with existentially bound variable(s)
**/
bool is_definite(clp_axiom* a){
  return(a->rhs->n_args <= 1);
}
bool is_existential(clp_axiom* a){
  return(a->is_existential);
}

bool is_fact(const clp_axiom* a){
  return(a->type == fact);
}
