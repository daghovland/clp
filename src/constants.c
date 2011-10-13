/* constants.c

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
/**
   Constants are based on pointer equality.
**/

#include "common.h"
#include "constants.h"
#include "theory.h"

constants init_constants(size_t n_vars){
  constants new_c;
  new_c.fresh = init_fresh_const(n_vars);

  new_c.size_constants = 2;
  new_c.constants = calloc_tester(new_c.size_constants, sizeof(char*));
  new_c.n_constants = 0;
  return new_c;
}

void destroy_constants(constants* c){
  free(c->constants);
}

constants copy_constants(constants * orig){
  constants copy = * orig;
  copy.constants = calloc_tester(orig->size_constants, sizeof(char*));
  memcpy(copy.constants, orig->constants, orig->size_constants * sizeof(char*));
  return copy;
}

/**
   We keep track of all constants, so equality on constants can be decided by pointer equality
**/
void insert_constant_name(constants * consts, const char* name){
  unsigned int i;
  assert(name != NULL && strlen(name) > 0);
  for(i = 0; i < consts->n_constants; i++){
    assert(consts->constants[i] != NULL);
    if(strcmp(consts->constants[i], name) == 0)
      return;
  }
  consts->n_constants++;
  if(consts->size_constants <= consts->n_constants){
    consts->size_constants *= 2;
    consts->constants = realloc_tester(consts->constants, consts->size_constants * sizeof(char*));
  }
  consts->constants[consts->n_constants - 1] = malloc_tester(strlen(name)+2);
  strcpy((char*) consts->constants[consts->n_constants - 1], name);
  return;
}
/*
constants_iter get_constants_iter(constants* state){
  return 0;
}

bool constants_iter_has_next(constants* state, constants_iter* iter){
  return *iter < state->n_constants;
}

const char* constants_iter_get_next(constants* state, constants_iter *iter){
  assert(constants_iter_has_next(state, iter));
  return state->constants[*iter++];
}
  
*/
 

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
  if(th->n_constants > 0){
    fprintf(stream, "Variables ");
    for(i = 0; i < th->n_constants; i++)
      fprintf(stream, "%s ", th->constants[i]);
    fprintf(stream, ": %s.\n", DOMAIN_SET_NAME);
  }
}
