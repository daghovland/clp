/* con_dis.c

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
#include "conjunction.h"
#include "disjunction.h"
#include "fresh_constants.h"
#include "rete.h"


/** 
    conjunction constructors and destructors
**/

conjunction* create_conjunction(const atom *at){
  conjunction * ret_val = malloc_tester(sizeof(conjunction));
  ret_val->size_args = 20;
  ret_val->args = calloc_tester(ret_val->size_args,  sizeof(atom*));
  ret_val->n_args = 1;
  (ret_val->args)[0] = at;
  ret_val->free_vars = init_freevars();
  ret_val->free_vars = free_atom_variables(at, ret_val->free_vars);
  ret_val->bound_vars =  init_freevars();
  ret_val->bound_vars = free_atom_variables(at, ret_val->bound_vars);
  ret_val->has_domain_pred  = at->pred->is_domain;
  return ret_val;
}

conjunction*create_empty_conjunction(){
  conjunction * ret_val = malloc_tester(sizeof(conjunction));
  ret_val->n_args = 0;
  ret_val->size_args = 0;
  ret_val->args = NULL;
  ret_val->free_vars = init_freevars();
  ret_val->bound_vars = init_freevars();
  ret_val->has_domain_pred = false;
  return ret_val;
}

conjunction* extend_conjunction(conjunction *conj, const atom* at){
  conj->n_args++;
  if(conj->n_args >= conj->size_args){
    conj->size_args *= 2;
    conj->args = realloc_tester(conj->args, (conj->size_args + 1) * sizeof(atom));
  }
  conj->args[conj->n_args - 1] = at;

  conj->free_vars = free_atom_variables(at, conj->free_vars);
  conj->bound_vars = free_atom_variables(at, conj->bound_vars);
  conj->has_domain_pred = conj->has_domain_pred || at->pred->is_domain;

  return conj;
}

void delete_conjunction(conjunction *conj){
  int i;
  assert(conj->n_args >= 0);
  for(i = 0; i < conj->n_args; i++)
    delete_atom((atom*) conj->args[i]);
  if(conj->args != NULL)
    free(conj->args);
  del_freevars(conj->bound_vars);
  del_freevars(conj->free_vars);
  free(conj);
}

/**
   disjunction constructors and destructors
**/

disjunction* create_disjunction(conjunction* disj){
  disjunction * ret_val = malloc_tester(sizeof(disjunction));
  ret_val->n_args = 1;
  ret_val->size_args = 100;
  ret_val->args = calloc_tester(ret_val->size_args, sizeof(conjunction*));
  ret_val->args[0] = disj;
  ret_val->free_vars = init_freevars();
  ret_val->has_domain_pred = disj->has_domain_pred;
  return ret_val;
}

disjunction*create_empty_disjunction(){
  disjunction * ret_val = malloc_tester(sizeof(disjunction));
  ret_val->n_args = 0;
  ret_val->size_args = 0;
  ret_val->args = NULL;
  ret_val->free_vars = init_freevars();
  ret_val->has_domain_pred = false;
  return ret_val;
}

disjunction* extend_disjunction(disjunction *disj, conjunction *conj){
  disj->n_args++;
  if(disj->n_args >= disj->size_args){
    disj->size_args *= 2;
    disj->args = realloc_tester(disj->args, disj->size_args * sizeof(conjunction*));
  }
  disj->args[disj->n_args-1] = conj;
  disj->has_domain_pred = disj->has_domain_pred || conj->has_domain_pred;
  return disj;
}

void delete_disjunction(disjunction *disj){
  int i;
  assert(disj->n_args >= 0);
  for(i = 0; i < disj->n_args; i++)
    delete_conjunction((conjunction*) disj->args[i]);
  del_freevars(disj->free_vars);
  if(disj->args != NULL)
    free(disj->args);
  free(disj);
}

/**
   Basic sanity testing
   Called from main method just after parsing
**/
bool test_conjunction(const conjunction* c){
  unsigned int i;
  assert(c->n_args <= c->size_args);
  for(i = 0; i < c->n_args; i++)
    test_atom(c->args[i]);
  return true;
}

bool test_disjunction(const disjunction* d){
  unsigned int i;
  assert(d->n_args <= d->size_args);
  for(i = 0; i < d->n_args; i++)
    assert(test_conjunction(d->args[i]));
  return true;
}


/**
   Creating the rete network

   Creates a new freevars structure for each conjunct. 
   This is passed to the rete node constructors and not touched afterwards

   Assumes that the given vars is a valid freevars structure which is not touched and not deleted

   The value "propagate" is false if the constructed alpha nodes should be "lazy" and not 
   pass on the substituions, but rather put them in a queue for the rule node to use if necessary
**/
rete_node * create_rete_conj_node(rete_net* net, 
				  const conjunction* con,  
				  const freevars* vars,
				  bool propagate,
				  size_t axiom_no){
  unsigned int i, j;
  rete_node * right_parent, *left_parent;

  assert(con->n_args > 0);
  assert(vars->size_vars >= vars->n_vars && vars->n_vars >= 0);

  left_parent = create_beta_left_root(axiom_no);
  for(i = 0; i < con->n_args; i++){
    freevars* copy = copy_freevars(vars);
    for(j = i; j < con->n_args; j++)
      copy = free_atom_variables(con->args[j], copy);
    right_parent = create_rete_atom_node(net, con->args[i], copy, propagate, axiom_no);
    left_parent = create_beta_and_node(net, left_parent, right_parent, copy, axiom_no);
  } // end for(i = 0; i < con->n_args; i++){
  return left_parent;
}

rete_node * create_rete_disj_node(rete_net* net, rete_node* left_parent, const disjunction* dis, size_t axiom_no){
  unsigned int i;
  rete_node* right_parent;
  assert(dis->n_args > 0);
  for(i = 0; i < dis->n_args; i++){
    right_parent = create_rete_conj_node(net, dis->args[i], copy_freevars(dis->free_vars), true, axiom_no);
    left_parent = create_beta_not_node(net, left_parent, right_parent, copy_freevars(dis->free_vars), axiom_no);
  } // end for(i = 0; i < dis->n_args; i++)
  return left_parent;
}
    

/**
   The free variables are returned
**/
freevars* free_conj_variables(const conjunction * con, freevars* vars){
  int i;
  for(i = 0; i < con->n_args; i++)
    vars = free_atom_variables(con->args[i], vars);
  return vars;
}


/** 
    Generic printing function
**/
void print_disj(const disjunction *dis, FILE* stream, char* or_sign, bool print_bindings, void (*print_conj)(const conjunction*, FILE*)){
  int i, j;
  bool print_parentheses;
  if(dis->n_args > 1 && print_bindings)
    fprintf(stream, "(");
  for(i = 0; i < dis->n_args; i++){
    const conjunction* con = dis->args[i];
    if(i > 0)
      fprintf(stream, "%s", or_sign);
    if(con->bound_vars->n_vars > 0){
      if(print_bindings){
	fprintf(stream, " ∃");
	for(j = 0; j < con->bound_vars->n_vars; j++){
	  fprintf(stream, "%s", con->bound_vars->vars[j]->name);
	  if(j+1 < con->bound_vars->n_vars)
	    fprintf(stream, ",");
	}
	fprintf(stream, ":");
      }
    }
    print_parentheses =  print_bindings && 
      ( 
       (dis->n_args > 1 && con->n_args > 1 )
       || con->bound_vars->n_vars > 0 
	);
    if(print_parentheses)
      fprintf(stream, "(");
    print_conj(con, stream);
    if(print_parentheses)
      fprintf(stream, ")");
  }
    if(dis->n_args > 1 && print_bindings)
    fprintf(stream, ")");

}

void print_conj(const conjunction * con, FILE* stream, char* and_sep, void (*print_atom)(const atom*, FILE*)){
  int i;
  for(i = 0; i < con->n_args-1; i++){
    print_fol_atom(con->args[i], stream);
    fprintf(stream, "%s", and_sep);
  }
  print_atom(con->args[i], stream);
  return;
}

/**
   Pretty-printing functions , print_fol gives standard fol output
**/
void print_fol_conj(const conjunction *con, FILE* stream){
  print_conj(con, stream, "/\\", print_fol_atom);
}

void print_fol_disj(const disjunction *dis, FILE* stream){
  print_disj(dis, stream, " \\/ ", true, print_fol_conj);
}

void print_dot_conj(const conjunction *con, FILE* stream){
  print_conj(con, stream, " ∧ ", print_fol_atom);
}

void print_dot_disj(const disjunction *dis, FILE* stream){
  print_disj(dis, stream, " ∨ ", true, print_dot_conj);
}

/**
   Print in geolog input format
**/
void print_geolog_disj(const disjunction *dis, FILE* stream){
  print_disj(dis, stream, ";", false, print_geolog_conj);
}

void print_geolog_conj(const conjunction *con, FILE* stream){
  print_conj(con, stream, ",", print_geolog_atom);
}
  
