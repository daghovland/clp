/**
   Only used by the parser to create the original list of predicates

   Identity of predicates is based on the string name and arity
**/

#include "common.h"
#include "theory.h"
#include "predicate.h"
#include "rete.h"


clp_predicate* _create_predicate(const char* name, size_t arity, bool is_equality, size_t pred_no){
  clp_predicate* p = malloc_tester(sizeof(clp_predicate));
  p->name = name;
  p->arity = arity;
  p->pred_no = pred_no;
  p->is_equality = is_equality;
  return p;
}

const clp_predicate* parser_new_predicate(theory* th, const char* new, size_t arity){
  unsigned int i;
  clp_predicate * p;
  if(strcmp(new, DOMAIN_SET_NAME) == 0){
    fprintf(stderr, "The predicate name \"%s\" cannot be used, as it is reserved for the domain of the prover.\n", DOMAIN_SET_NAME);
    exit(EXIT_FAILURE);
  }
    
  for(i = 0; i < th->n_predicates; i++){
    p = th->predicates[i];
    if(strcmp(p->name, new) == 0 && arity == p->arity)
      return p;
  }
  p = _create_predicate(new, arity, (strcmp(new, "=") == 0 && arity == 2), th->n_predicates);
  if(th->n_predicates+1 >= th->size_predicates){
    th->size_predicates *= 2;
    th->predicates = realloc_tester(th->predicates, sizeof(clp_predicate) * th->size_predicates);
  }
  th->predicates[th->n_predicates] = p;
  th->n_predicates++;  
  return p;
}


void print_coq_predicate(const clp_predicate* p, FILE* stream){
  unsigned int j;
  fprintf(stream, "Variable %s : ", p->name);
  for(j = 0; j < p->arity; j++)
    fprintf(stream, "%s -> ", DOMAIN_SET_NAME);
  fprintf(stream, "Prop.\n");
}

bool test_predicate(const clp_predicate* p){
  assert(strlen(p->name) > 0);
  return true;
}
