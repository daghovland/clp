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

clp_conjunction* create_conjunction(const clp_atom *at){
  clp_conjunction * ret_val = malloc_tester(sizeof(clp_conjunction));
  ret_val->size_args = 20;
  ret_val->args = calloc_tester(ret_val->size_args,  sizeof(clp_atom*));
  ret_val->n_args = 1;
  (ret_val->args)[0] = at;
  ret_val->free_vars = init_freevars();
  ret_val->free_vars = free_atom_variables(at, ret_val->free_vars);
  ret_val->n_equalities = (at->pred->is_equality) ? 1 : 0;
  ret_val->bound_vars =  init_freevars();
  ret_val->bound_vars = free_atom_variables(at, ret_val->bound_vars);
  return ret_val;
}

clp_conjunction* create_empty_conjunction(theory* th){
  clp_conjunction * ret_val = malloc_tester(sizeof(clp_conjunction));
  ret_val->n_args = 1;
  ret_val->size_args = 1;
  ret_val->args =  calloc_tester(ret_val->size_args,  sizeof(clp_atom*));
  (ret_val->args)[0] = create_prop_variable("true", th);
  ret_val->free_vars = init_freevars();
  ret_val->bound_vars = init_freevars();
  return ret_val;
}

void increase_conjunction_size(clp_conjunction *conj){
  conj->n_args++;
  if(conj->n_args >= conj->size_args){
    conj->size_args *= 2;
    conj->args = realloc_tester(conj->args, (conj->size_args + 1) * sizeof(clp_atom));
  }
}

clp_conjunction* extend_conjunction(clp_conjunction *conj, const clp_atom* at){
  increase_conjunction_size(conj);
  conj->args[conj->n_args - 1] = at;
  conj->free_vars = free_atom_variables(at, conj->free_vars);
  conj->bound_vars = free_atom_variables(at, conj->bound_vars);
  if(at->pred->is_equality)
    conj->n_equalities++;
  return conj;
}

void delete_conjunction(clp_conjunction *conj){
  int i;
  assert(conj->n_args >= 0);
  for(i = 0; i < conj->n_args; i++)
    delete_atom((clp_atom*) conj->args[i]);
  if(conj->args != NULL)
    free(conj->args);
  del_freevars(conj->bound_vars);
  del_freevars(conj->free_vars);
  free(conj);
}

/**
   disjunction constructors and destructors
**/

clp_disjunction* create_disjunction(clp_conjunction* disj){
  clp_disjunction * ret_val = malloc_tester(sizeof(clp_disjunction));
  ret_val->n_args = 1;
  ret_val->size_args = 100;
  ret_val->args = calloc_tester(ret_val->size_args, sizeof(clp_conjunction*));
  ret_val->args[0] = disj;
  ret_val->free_vars = init_freevars();
  return ret_val;
}

clp_disjunction* create_empty_disjunction(){
  clp_disjunction * ret_val = malloc_tester(sizeof(clp_disjunction));
  ret_val->n_args = 0;
  ret_val->size_args = 0;
  ret_val->args = NULL;
  ret_val->free_vars = init_freevars();
  return ret_val;
}

clp_disjunction* extend_disjunction(clp_disjunction *disj, clp_conjunction *conj){
  disj->n_args++;
  if(disj->n_args >= disj->size_args){
    disj->size_args *= 2;
    disj->args = realloc_tester(disj->args, disj->size_args * sizeof(clp_conjunction*));
  }
  disj->args[disj->n_args-1] = conj;
  return disj;
}

void delete_disjunction(clp_disjunction *disj){
  int i;
  assert(disj->n_args >= 0);
  for(i = 0; i < disj->n_args; i++)
    delete_conjunction((clp_conjunction*) disj->args[i]);
  del_freevars(disj->free_vars);
  if(disj->args != NULL)
    free(disj->args);
  free(disj);
}

/**
   Basic sanity testing
   Called from main method just after parsing
**/
bool test_conjunction(const clp_conjunction* c, const constants* cs){
  unsigned int i;
  assert(c->n_args <= c->size_args);
  for(i = 0; i < c->n_args; i++)
    test_atom(c->args[i], cs);
  return true;
}

bool test_disjunction(const clp_disjunction* d, const constants* cs){
  unsigned int i;
  assert(d->n_args <= d->size_args);
  for(i = 0; i < d->n_args; i++)
    assert(test_conjunction(d->args[i], cs));
  return true;
}

/**
   Only called from fix_equality_vars below

   Checks that if the term t is a variable, then it either occurs in left, 
   or a dom(t->val.var) is inserted into the conjunction
**/
unsigned int test_equality_var(clp_conjunction* con, theory* th, unsigned int i, freevars* left, const clp_term* t){
  if(t->type == variable_term && !is_in_freevars(left, t->val.var)){
    unsigned int j;
    term_list* tl = create_term_list(copy_term(t));
    increase_conjunction_size(con);
    for(j = i; j < con->n_args; j++)
      con->args[j+1] = con->args[j];
    con->args[i] = parser_create_atom(DOMAIN_PRED_NAME, tl, th);
    i++;
    free_term_variables(t, left);
  }
  return i;
}


/**
   Checks that equalitites are not the left-most instance of any variable. 
   Adds dom() for variables only in equalities.

   Called from extend_theory
**/
void fix_equality_vars(clp_conjunction *con, theory* th){
  freevars* left = init_freevars();
  unsigned int i;
  for(i = 0; i < con->n_args; i++){
    if(con->args[i]->pred->is_equality){
      const clp_term* t1 = con->args[i]->args->args[0];
      const clp_term* t2 = con->args[i]->args->args[1];
      i = test_equality_var(con, th, i, left, t1);
      i = test_equality_var(con, th, i, left, t2);
    }
  }
  //  del_freevars(left);
}

/**
   Creating the rete network

   Creates a new freevars structure for each conjunct. 
   This is passed to the rete node constructors and not touched afterwards

   Assumes that the given vars is a valid freevars structure which is not touched and not deleted

   The value "propagate" is false if the constructed alpha nodes should be "lazy" and not 
   pass on the substitutions, but rather put them in a queue for the rule node to use if necessary
**/
rete_node * create_rete_conj_node(rete_net* net, 
				  const clp_conjunction* con,  
				  const freevars* vars,
				  bool propagate,
				  bool in_positive_lhs_part,
				  unsigned int axiom_no){
  unsigned int i, j;
  rete_node * right_parent, *left_parent;

  assert(con->n_args > 0);
  assert(vars->size_vars >= vars->n_vars && vars->n_vars >= 0);

  left_parent = create_beta_left_root(axiom_no);
  for(i = 0; i < con->n_args; i++){
    freevars* copy = copy_freevars(vars);
    for(j = i; j < con->n_args; j++)
      copy = free_atom_variables(con->args[j], copy);
    if(con->args[i]->pred->is_equality)
      left_parent = create_rete_equality_node(net, con->args[i]->args->args[0], con->args[i]->args->args[1], left_parent, copy, in_positive_lhs_part, axiom_no);
    else {
      right_parent = create_rete_atom_node(net, con->args[i], copy, propagate, in_positive_lhs_part, axiom_no);
      left_parent = create_beta_and_node(net, left_parent, right_parent, copy, in_positive_lhs_part, axiom_no);
    }
  } // end for(i = 0; i < con->n_args; i++){
  return left_parent;
}

/**
   The newer beta-not implementaion, inserts beta-not nodes as much to the left as possible
   taking into account that all variables in the lhs must occur to the left of the beta-not node
**/
rete_node * insert_beta_not_nodes(rete_net* net, 
				  const clp_conjunction* con, 
				  const clp_disjunction* dis, 
				  rete_node* beta_node, 
				  unsigned int axiom_no){
  unsigned int i, j;
  freevars  *conj_freevars = init_freevars();
  bool *disjunctions_done = calloc_tester(dis->n_args, sizeof(bool));
  for(j = 0; j < dis->n_args; j++)
    disjunctions_done[j] = false;
  assert(beta_node->n_children == 0);
  while(beta_node->type != beta_root)
    beta_node = (rete_node*) beta_node->left_parent;
  for(i = 0; i < con->n_args;  i++){
    assert(beta_node->n_children == 1);
    assert(beta_node->children[0] != NULL);
    beta_node = (rete_node*) beta_node->children[0];
    assert(beta_node->type == beta_and || beta_node->type == equality_node);
    conj_freevars = free_atom_variables(con->args[i], conj_freevars);
    for(j = 0; j < dis->n_args; j++){
      if(!disjunctions_done[j]){
	if(freevars_included(dis->args[j]->free_vars, conj_freevars)){
	  rete_node* child = NULL;
	  if(beta_node->n_children == 1){
	    child = (rete_node*) beta_node->children[0];
	    assert(child != NULL);
	  }
	  beta_node->n_children = 0;
	  rete_node * right_parent = create_rete_conj_node(net, dis->args[j], copy_freevars(dis->free_vars), true, false, axiom_no);
	  rete_node* left_parent = create_beta_not_node(net, beta_node, right_parent, copy_freevars(dis->free_vars), axiom_no);
	  assert(left_parent->size_children > 0);
	  assert(left_parent->n_children == 0);
	  if(child != NULL){
	    child->left_parent = left_parent;
	    left_parent->children[0] = child;
	    left_parent->n_children = 1;
	  } else {
	    assert(beta_node->n_children == 1);
	  }
	  beta_node = left_parent;
	  disjunctions_done[j] = true;
	}
      }
    }
  }
  beta_node->n_children = 0;
  return beta_node;
}

/**
   Used by the original "beta-not" implementation, where the beta-not comes last, just before the rule node.
   
**/
rete_node * create_rete_disj_node(rete_net* net, rete_node* left_parent, const clp_disjunction* dis, unsigned int axiom_no){
  unsigned int i;
  rete_node* right_parent;
  assert(dis->n_args > 0);
  for(i = 0; i < dis->n_args; i++){
    right_parent = create_rete_conj_node(net, dis->args[i], copy_freevars(dis->free_vars), true, false, axiom_no);
    left_parent = create_beta_not_node(net, left_parent, right_parent, copy_freevars(dis->free_vars), axiom_no);
  } // end for(i = 0; i < dis->n_args; i++)
  return left_parent;
}
    

/**
   The free variables are returned
**/
freevars* free_conj_variables(const clp_conjunction * con, freevars* vars){
  int i;
  for(i = 0; i < con->n_args; i++)
    vars = free_atom_variables(con->args[i], vars);
  return vars;
}


/** 
    Generic printing function
**/
void print_disj(const clp_disjunction *dis, const constants* cs, FILE* stream, char* or_sign, char* exist_sign, char* exist_end, bool print_bindings, void (*print_conj)(const clp_conjunction*, const constants*, FILE*)){
  int i, j;
  bool print_parentheses;
  if(dis->n_args > 1 && print_bindings)
    fprintf(stream, "(");
  for(i = 0; i < dis->n_args; i++){
    const clp_conjunction* con = dis->args[i];
    if(i > 0)
      fprintf(stream, "%s", or_sign);
    if(con->bound_vars->n_vars > 0){
      if(print_bindings){
	fprintf(stream, "%s",  exist_sign);
	for(j = 0; j < con->bound_vars->n_vars; j++){
	  fprintf(stream, "%s", con->bound_vars->vars[j]->name);
	  if(j+1 < con->bound_vars->n_vars)
	    fprintf(stream, ", ");
	}
	fprintf(stream, "%s ", exist_end);
      }
    }
    print_parentheses =  print_bindings && 
      ( 
       (dis->n_args > 1 && con->n_args > 1 )
       || con->bound_vars->n_vars > 0 
	);
    if(print_parentheses)
      fprintf(stream, "(");
    print_conj(con, cs, stream);
    if(print_parentheses)
      fprintf(stream, ")");
  }
    if(dis->n_args > 1 && print_bindings)
    fprintf(stream, ")");

}

void print_conj(const clp_conjunction * con, const constants* cs, FILE* stream, char* and_sep, void (*print_atom)(const clp_atom*, const constants*, FILE*)){
  int i;
  for(i = 0; i < con->n_args-1; i++){
    print_fol_atom(con->args[i], cs, stream);
    fprintf(stream, "%s", and_sep);
  }
  print_atom(con->args[i], cs, stream);
  return;
}

/**
   Pretty-printing functions , print_fol gives standard fol output
**/
void print_fol_conj(const clp_conjunction *con, const constants* cs, FILE* stream){
  print_conj(con, cs, stream, "/\\", print_fol_atom);
}
void print_clpl_conj(const clp_conjunction *con, const constants* cs, FILE* stream){
  print_conj(con, cs, stream, ", ", print_fol_atom);
}

void print_fol_disj(const clp_disjunction *dis, const constants* cs, FILE* stream){
  print_disj(dis, cs, stream, " \\/ "," ∃",  ":", true, print_fol_conj);
}

void print_dot_conj(const clp_conjunction *con, const constants* cs, FILE* stream){
  print_conj(con, cs, stream, " ∧ ", print_fol_atom);
}

void print_dot_disj(const clp_disjunction *dis, const constants* cs, FILE* stream){
  print_disj(dis, cs, stream, " ∨ ", " ∃", ":", true, print_dot_conj);
}

/**
   Outputs in coq format. The domain predicate must then not be printed
   Returns true iff there were some non-domain atoms. 
   There is output iff return value is true.
**/
bool print_coq_conj(const clp_conjunction* con, const constants* cs, FILE* stream){
  unsigned int i;
  bool has_printed_one = false;
  for(i = 0; i < con->n_args; i++){
    if(has_printed_one)
      fprintf(stream, " /\\ ");
    print_coq_atom(con->args[i], cs, stream);
    has_printed_one = true;
  }
  return has_printed_one;
}
void print_coq_disj(const clp_disjunction* dis, const constants* cs, FILE* stream){
  freevars* ev;
  unsigned int i, j;
  for(i = 0; i < dis->n_args; i++){
    clp_conjunction* arg = dis->args[i];
    if(arg->n_args > 0 || arg->is_existential)
      fprintf(stream, "(");
    if(arg->is_existential){
      ev = arg->bound_vars;
      for(j = 0; j < ev->n_vars; j++)
	fprintf(stream, "exists %s, ", ev->vars[j]->name);
    }
    if(!print_coq_conj(arg, cs, stream))
      fprintf(stderr, "con_dis.c: print_coq_disj: Warning: A disjunct in an rhs contained only domain predicates. This might not make sense.\n");
    if(arg->n_args > 0 || arg->is_existential)
      fprintf(stream, ")");
    if(i < dis->n_args - 1)
      fprintf(stream, " \\/ ");
  }
} 
/**
   Print in geolog input format
**/
void print_geolog_disj(const clp_disjunction *dis, const constants* cs, FILE* stream){
  print_disj(dis, cs, stream, ";", " ∃", ":", false, print_geolog_conj);
}
void print_clpl_disj(const clp_disjunction *dis, const constants* cs, FILE* stream){
  print_disj(dis, cs, stream, ";", " ∃", ":", false, print_clpl_conj);
}

void print_geolog_conj(const clp_conjunction *con, const constants* cs, FILE* stream){
  print_conj(con, cs, stream, ",", print_geolog_atom);
}

/**
   Print in TPTP format
**/
void print_tptp_disj(const clp_disjunction *dis, const constants* cs, FILE* stream){
  print_disj(dis, cs, stream, "|", " ? [", " ]: ", true, print_tptp_conj);
}

void print_tptp_conj(const clp_conjunction *con, const constants* cs, FILE* stream){
  if(con->n_args > 1)
    fprintf(stream, "(");
  print_conj(con, cs, stream, " & ", print_fol_atom);
  if(con->n_args > 1)
    fprintf(stream, ")");
}
  
