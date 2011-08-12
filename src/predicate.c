/**
   Only used by the parser to create the original list of predicates

   Identity of predicates is based on the string name and arity
**/

#include "common.h"
#include "theory.h"
#include "predicate.h"
#include "rete.h"


predicate* _create_predicate(const char* name, size_t arity, size_t pred_no, bool is_domain){
  predicate* p = malloc_tester(sizeof(predicate));
  p->name = name;
  p->arity = arity;
  p->pred_no = pred_no;
  p->is_domain = is_domain;
  return p;
}

const predicate* parser_new_predicate(theory* th, const char* new, size_t arity){
  unsigned int i;
  predicate * p;
  bool is_domain;
  for(i = 0; i < th->n_predicates; i++){
    p = th->predicates[i];
    if(strcmp(p->name, new) == 0 && arity == p->arity)
      return p;
  }
  is_domain = (strcmp(new, DOMAIN_PREDICATE_NAME) == 0);
  p = _create_predicate(new, arity, th->n_predicates, is_domain);
  if(th->n_predicates+1 >= th->size_predicates){
    th->size_predicates *= 2;
    th->predicates = realloc_tester(th->predicates, sizeof(predicate) * th->size_predicates);
  }
  th->predicates[th->n_predicates] = p;
  th->n_predicates++;  
  return p;
}

bool test_predicate(const predicate* p){
  assert(strlen(p->name) > 0);
  return true;
}
