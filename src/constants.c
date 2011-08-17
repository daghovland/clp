/**
   Only used by the parser to create the original list of constants

   Identity of constants is based on the string name
**/

#include "common.h"
#include "constants.h"
#include "theory.h"

const char* parser_new_constant(theory* th, const char* new){
  unsigned int i;
  const char* c;
  for(i = 0; i < th->n_constants; i++){
    c = th->constants[i];
    if(strcmp(c, new) == 0)
      return c;
  }
  if(th->n_constants+1 >= th->size_constants){
    th->size_constants *= 2;
    th->constants = realloc_tester(th->constants, sizeof(char*) * th->size_constants);
  }
  th->constants[th->n_constants] = new;
  th->n_constants++;  
  return new;
}


void print_coq_constants(const theory* th, FILE* stream){
  unsigned int i;
  fprintf(stream, "Variables ");
  for(i = 0; i < th->n_constants; i++)
    fprintf(stream, "%s ", th->constants[i]);
  fprintf(stream, ": dom\n");
}
