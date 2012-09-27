/* theory.c

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
#include "theory.h"
#include "fresh_constants.h"
#include "rete.h"
#include "substitution.h"
#include <math.h>
#include <inttypes.h>
#ifdef HAVE_PTHREAD
#include <pthread.h>
#endif


theory* create_theory(void){  
  theory * ret_val = malloc_tester(sizeof(theory));

  ret_val->size_predicates = 100;
  ret_val->n_predicates = 0;
  ret_val->predicates = calloc_tester(sizeof(clp_predicate*),ret_val->size_predicates);


  ret_val->size_axioms = 100;
  ret_val->axioms = calloc_tester(ret_val->size_axioms, sizeof(clp_axiom*));
  ret_val->n_axioms =  0;

  ret_val->size_init_model = 100;
  ret_val->init_model = calloc_tester(ret_val->size_axioms, sizeof(clp_conjunction*));
  ret_val->n_init_model =  0;

  ret_val->size_func_names = 100;
  ret_val->func_names = calloc_tester(ret_val->size_func_names, sizeof(char*));
  ret_val->n_func_names =  0;


  ret_val->constants = init_constants(100);

  ret_val->vars = init_freevars();
  ret_val->name = NULL;
  ret_val->has_name = false;
  ret_val->max_lhs_conjuncts = 0;
  ret_val->max_rhs_conjuncts = 0;
  ret_val->max_rhs_disjuncts = 0;
#ifndef NDEBUG
  ret_val->finalized = false;
#endif
  return ret_val;
}

void extend_init_model(theory* th, clp_conjunction* fact){
  th->n_init_model++;
  if(th->n_init_model >= th->size_init_model){
    th->size_init_model *= 2;
    th->init_model = realloc_tester(th->init_model, th->size_init_model * sizeof(clp_axiom*));
  }
  th->init_model[th->n_init_model-1] = fact;
}

void extend_theory(theory *th, clp_axiom *ax){
  unsigned int i;
  if(ax->type == fact && ax->rhs->n_args == 1  && !ax->rhs->args[0]->is_existential){
    extend_init_model(th, ax->rhs->args[0]);
    return;
  }
  ax->axiom_no = th->n_axioms;

  assert(!th->finalized);
  if(!ax->has_name){
    char* axname = malloc_tester(10 + abs(log10(th->n_axioms)));
    sprintf(axname, "axiom_%i", th->n_axioms);
    set_axiom_name(ax, axname);
  }

  th->n_axioms++;
  if(th->n_axioms >= th->size_axioms){
    th->size_axioms *= 2;
    th->axioms = realloc_tester(th->axioms, th->size_axioms * sizeof(clp_axiom*));
  }
  th->axioms[th->n_axioms-1] = ax;
  if(ax->lhs->n_args > th->max_lhs_conjuncts)
    th->max_lhs_conjuncts = ax->lhs->n_args; 
  if(ax->rhs->n_args > th->max_rhs_disjuncts)
    th->max_rhs_disjuncts = ax->rhs->n_args; 
  for(i = 0; i < ax->rhs->n_args; i++){
    if(ax->rhs->args[i]->n_args > th->max_rhs_conjuncts)
      th->max_rhs_conjuncts = ax->rhs->args[i]->n_args; 
  }
}

void delete_theory(theory* t){
  int i;
  for(i = 0; i < t->n_axioms; i++)
    delete_axiom((clp_axiom*) t->axioms[i]);
  free(t->axioms);
  for(i = 0; i < t->n_predicates; i++)
    free(t->predicates[i]);
  free(t->predicates);
  del_freevars(t->vars);
  if(t->name != NULL)
    free(t->name);
  free(t);
}


/**
   From the "name" predicate in CL.pl format
**/
void set_theory_name(theory *th, const char* name){
  unsigned int i, j;
  assert(th->name == NULL && th->has_name == false);
  assert(strlen(name) >= 0);

  th->has_name = true;
  th->name = malloc_tester(strlen(name) + 1);
  
  for(i = 0, j = 0; name[i] != '\0'; i++){
    if( name[i] != '+' && name[i] != '.' && name[i] != '\'' && name[i] != '\"'){
      th->name[j] = name[i];
      j++;
    }
  }
  th->name[j] = '\0';
  
  assert(j == strlen(th->name));  
}

/**
   Checks whether a name has been set for the theory
   For CL.pl format, this is set by the name predicate in the theory file. 
   In geolog, we must in stead use the filename
**/
bool has_theory_name(const theory* th){
  return th->has_name;
}

/**
   This function must be called after all axioms are added.
**/
void finalize_theory(theory* th){
  th->sub_size_info = init_sub_size_info(th->vars->n_vars, th->max_lhs_conjuncts);
#ifndef NDEBUG
  th->finalized = true;
#endif
}

/**
   Creates new rete network for the whole theory

**/
rete_net* create_rete_net(const theory* th, unsigned long maxsteps, bool existdom, strategy strat, bool lazy, bool coq, bool use_beta_not, bool factset_lhs, bool print_model, bool all_disjuncts, bool verbose, bool multithread_rete){
#ifdef HAVE_PTHREAD
  pthread_mutexattr_t p_attr;
#endif
  unsigned int i;
  rete_net* net = init_rete(th, maxsteps, lazy, coq, multithread_rete);
  assert(th->finalized);
  net->rule_nodes = calloc_tester(th->n_axioms, sizeof(rete_node*));
  for(i = 0; i < th->n_axioms; i++){
#ifndef NDEBUG
    const clp_axiom* ax = th->axioms[i];
#endif
    assert(!(ax->type == fact && ax->rhs->n_args == 1 && !ax->rhs->args[0]->is_existential));
    net->rule_nodes[i] = create_rete_axiom_node(net, th->axioms[i], i, use_beta_not);
  }
  net->th = th;
  net->existdom = existdom;
  net->strat = strat;
  net->treat_all_disjuncts = all_disjuncts;
  net->use_beta_not = use_beta_not;
  net->factset_lhs = factset_lhs;
#ifdef HAVE_PTHREAD
  pthread_mutexattr_init(&p_attr);
#ifndef NDEBUG
  pthread_mutexattr_settype(&p_attr, PTHREAD_MUTEX_ERRORCHECK_NP);
#endif
  net->sub_mutexes = calloc_tester(net->n_subs, sizeof(pthread_mutex_t));
  for(i = 0; i < net->n_subs; i++)
    pthread_mutex_init(&net->sub_mutexes[i], &p_attr);

  pthread_mutexattr_destroy(&p_attr);
#endif

  return net;
}
   
/**
   Called from the main function to test that the theory
   is created correctly. A kind of "unit testing" 
**/
bool test_theory(const theory* t){
  unsigned int i;
  for(i = 0; i < t->n_axioms; i++)
    assert(test_axiom(t->axioms[i], i, t->constants));
  for(i = 0; i < t->n_predicates; i++){
    assert(t->predicates[i]->pred_no == i);
    assert(test_predicate(t->predicates[i]));
  }
  return true;
}
   
/**
   Prints a proof in coq format. Not finished.
**/
void print_coq_proof_intro(const theory* th, const constants* cs, FILE* stream){
  unsigned int i, j;
  fprintf(stream, "Section %s.\n\n", th->name);
  fprintf(stream, "Let false := False.\nLet false_ind := False_ind.\n\n");

  fprintf(stream, "Variable goal : Prop.\n");
  fprintf(stream, "Variable %s : Set.\n", DOMAIN_SET_NAME);

  for(i = 0; i < th->n_predicates; i++){
    if(! th->predicates[i]->is_equality)
      print_coq_predicate(th->predicates[i], stream);
  }

  print_coq_constants(th->constants, stream);
  fprintf(stream, "\n");
  
  for(i = 0; i < th->n_init_model; i++){
    clp_conjunction *con = th->init_model[i];
    assert(con->n_args > 0);
    for(j = 0; j < con->n_args; j++){
      fprintf(stream, "Hypothesis init_model_%i_%i : ", i, j);
      print_coq_atom(th->init_model[i]->args[j], cs, stream);
      fprintf(stream, ".\n");
    }
  }

  for(i = 0; i < th->n_axioms; i++)
    print_coq_axiom(th->axioms[i], cs, stream);
  
  fprintf(stream, "Theorem %s : goal.\n", th->name);
  fprintf(stream, "Proof.\n");
}

/**
   Prints a proof in coq format. Not finished.
**/
void print_coq_proof_ending(const theory* th, FILE* stream){
  fprintf(stream, "Qed.\n\n");
  fprintf(stream, "End %s.\n", th->name);
}
/**
   Generic pretty printing function
**/
void print_theory(const theory * th, const constants* cs, FILE* stream, void (*print_axiom)(const clp_axiom*, const constants*, FILE*) ,  void (*print_conj)(const clp_conjunction*, const constants*, FILE*), const char* init_start, const char* init_end, const char* and_sign ){
  int i;
  if(th->n_init_model > 0){
    fprintf(stream, "%s", init_start);
    for(i = 0; i < th->n_init_model; i++){
      print_conj(th->init_model[i], cs, stream);
      if(i +1 < th->n_init_model)
	fprintf(stream, " %s ", and_sign);
    }
    fprintf(stream, "%s \n", init_end);
  }
  for(i = 0; i < th->n_axioms; i++){
    print_axiom(th->axioms[i], cs, stream);
    fprintf(stream, "\n");
  }
}

/**
   Print in fol format
**/
void print_fol_theory(const theory * th, const constants* cs, FILE* stream){
  print_theory(th, cs, stream, print_fol_axiom, print_fol_conj, "", ".", "/\\");
}

/**
   Print in geolog input format
**/
void print_geolog_theory(const theory* th, const constants* cs, FILE* stream){
  print_theory(th, cs, stream, print_geolog_axiom, print_geolog_conj, "true => ", ".", " , ");
}
/**
   Print in geolog input format
**/
void print_clpl_theory(const theory* th, const constants* cs, FILE* stream){
  print_theory(th, cs, stream, print_clpl_axiom, print_clpl_conj, "", ".", " , ");
}

/**
   Print in TPTP format
**/
void print_tptp_theory(const theory* th, const constants* cs, FILE* stream){
  print_theory(th, cs, stream, print_tptp_axiom, print_tptp_conj, "fof(initial_model, axiom, ", ").", " & ");
  fprintf(stream, "fof(goal_to_be_proved,conjecture,( goal )).\n");
}
  
