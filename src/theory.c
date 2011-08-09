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


theory* create_theory(void){  
  theory * ret_val = malloc_tester(sizeof(theory));

  ret_val->size_predicates = 100;
  ret_val->n_predicates = 0;
  ret_val->predicates = calloc_tester(sizeof(predicate*),ret_val->size_predicates);

  ret_val->size_axioms = 100;
  ret_val->axioms = calloc_tester(ret_val->size_axioms, sizeof(axiom));
  ret_val->n_axioms =  0;

  ret_val->size_func_names = 100;
  ret_val->func_names = calloc_tester(ret_val->size_func_names, sizeof(char*));
  ret_val->n_func_names =  0;


  ret_val->size_constants = 100;
  ret_val->constants = calloc_tester(ret_val->size_constants, sizeof(char*));
  ret_val->n_constants =  0;

  ret_val->vars = init_freevars();

  ret_val->max_lhs_conjuncts = 0;

  return ret_val;
}

void extend_theory(theory *th, axiom *ax){
  ax->axiom_no = th->n_axioms;
  th->n_axioms++;
  if(th->n_axioms >= th->size_axioms){
    th->size_axioms *= 2;
    th->axioms = realloc_tester(th->axioms, th->size_axioms * sizeof(axiom));
  }
  th->axioms[th->n_axioms-1] = ax;
  if(ax->lhs->n_args > th->max_lhs_conjuncts)
    th->max_lhs_conjuncts = ax->lhs->n_args;
}

void delete_theory(theory* t){
  int i;
  for(i = 0; i < t->n_axioms; i++)
    delete_axiom((axiom*) t->axioms[i]);
  free(t->axioms);
  for(i = 0; i < t->n_predicates; i++)
    free(t->predicates[i]);
  free(t->predicates);
  del_freevars(t->vars);
  free(t);
}

/**
   Creates new rete network for the whole theory
**/
rete_net* create_rete_net(const theory* th, unsigned long maxsteps, bool existdom, strategy strat){
  unsigned int i;
  rete_net* net = init_rete(th, maxsteps);
  for(i = 0; i < th->n_axioms; i++)
    create_rete_axiom_node(net, th->axioms[i], i);
  net->th = th;
  net->existdom = existdom;
  net->strat = strat;
  return net;
}
   
/**
   Called from the main function to test that the theory
   is created correctly. A kind of "unit testing" 
**/
bool test_theory(const theory* t){
  unsigned int i;
  for(i = 0; i < t->n_axioms; i++)
    assert(test_axiom(t->axioms[i], i));
  for(i = 0; i < t->n_predicates; i++){
    assert(t->predicates[i]->pred_no == i);
    assert(test_predicate(t->predicates[i]));
  }
  return true;
}


/**
   Generic pretty printing function
**/
void print_theory(const theory * th, FILE* stream, void (*print_axiom)(const axiom*, FILE*) ){
  int i;
  for(i = 0; i < th->n_axioms; i++){
    print_axiom(th->axioms[i], stream);
    fprintf(stream, "\n");
  }
}

/**
   Print in fol format
**/
void print_fol_theory(const theory * th, FILE* stream){
  print_theory(th, stream, print_fol_axiom);
}

/**
   Print in geolog input format
**/
void print_geolog_theory(const theory* th, FILE* stream){
  print_theory(th, stream, print_geolog_axiom);
}
  
