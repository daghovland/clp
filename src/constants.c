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

constants* init_constants(unsigned int init_size){
  constants * new_c = malloc_tester(sizeof(constants));
  new_c->fresh = init_fresh_const(init_size);

  new_c->size_constants = init_size;
  new_c->constants = calloc_tester(new_c->size_constants, sizeof(constant));
  new_c->n_constants = 0;
  
  return new_c;
}

void destroy_constants(constants* c){
  free(c->constants);
  free(c);
}

constants* copy_constants(const constants * orig){
  constants* copy = malloc_tester(sizeof(constants));
  copy = memcpy(copy, orig, sizeof(constants));
  copy->constants = calloc_tester(orig->size_constants, sizeof(constant));
  memcpy(copy->constants, orig->constants, orig->size_constants * sizeof(constant));
  return copy;
}

const char* get_constant_name(unsigned int c, const constants* cs){
  return cs->constants[c].name;
}

/**
   
**/
unsigned int insert_constant_name(constants * consts, const char* name){
  unsigned int new_const_ind;
  constant* new_c;
  assert(name != NULL && strlen(name) > 0);
  new_const_ind = consts->n_constants;
  consts->n_constants++;
  if(consts->size_constants <= consts->n_constants){
    consts->size_constants *= 2;
    consts->constants = realloc_tester(consts->constants, consts->size_constants * sizeof(constant));
  }
  new_c = & consts->constants[new_const_ind];
  new_c->name = malloc_tester(strlen(name)+2);
  strcpy((char*) new_c->name, name);
  new_c->rank = 0;
  new_c->parent = new_const_ind;
  return new_const_ind;
}

/**
   Creates a new domain element. Called from instantiate.c
**/
const term* get_fresh_constant(variable* var, constants* constants){
  unsigned int new_const_ind;
  char* name;
  const term* t;
  assert(constants->fresh != NULL);
  unsigned int const_no = next_fresh_const_no(constants->fresh, var->var_no);
  constants->n_constants ++;
  name = calloc_tester(sizeof(char), strlen(var->name) + 20);
  sprintf(name, "%s_%i", var->name, const_no);
  new_const_ind = insert_constant_name(constants, name);
  t = prover_create_constant_term(new_const_ind);
  return t;
}

/**
   Part of union-find alg. 
   http://en.wikipedia.org/wiki/Disjoint-set_data_structure
**/
unsigned int find_constant_root(unsigned int c, constants* cs){
  if(cs->constants[c].parent != c)
    cs->constants[c].parent = find_constant_root(cs->constants[c].parent, cs);
  return cs->constants[c].parent;
}


/**
   Part of union-find alg. 
   http://en.wikipedia.org/wiki/Disjoint-set_data_structure
**/
void union_constants(unsigned int c1, unsigned int c2, constants* consts){
  unsigned int c1_root = find_constant_root(c1, consts);
  unsigned int c2_root = find_constant_root(c2, consts);
  if(c1_root == c2_root) 
    return;
  if(consts->constants[c1_root].rank < consts->constants[c2_root].rank)
    consts->constants[c1_root].parent = c2_root;
  else if(consts->constants[c1_root].rank > consts->constants[c2_root].rank)
    consts->constants[c2_root].parent = c1_root;
  else {
    consts->constants[c2_root].parent = c1_root;
    consts->constants[c1_root].rank++;
  }
}


/**
   Part of union-find alg. 
   http://en.wikipedia.org/wiki/Disjoint-set_data_structure
**/
bool equal_constants(unsigned int c1, unsigned int c2, constants* consts){
  unsigned int p1 = find_constant_root(c1, consts);
  unsigned int p2 = find_constant_root(c2, consts);
  return p1 == p2;
}

/**
   Called from rete_state when in a disjunctive split. 
**/
constants* backup_constants(const constants* orig){
  return copy_constants(orig);
}

/**
   The prover iterates over all constants to get 
   the domain
**/
constants_iter get_constants_iter(constants* state){
  return 0;
}

bool constants_iter_has_next(constants* state, constants_iter* iter){
  return *iter < state->n_constants;
}

constant* constants_iter_get_next(constants* state, constants_iter *iter){
  assert(constants_iter_has_next(state, iter));
  return & state->constants[*iter++];
}

void destroy_constants_iter(constants_iter iter){
  return;
}

 
/**
   Called from the parser (via create_constant_term in atom_and_term.c) 
   when seeing a constant
**/
unsigned int parser_new_constant(constants* cs, const char* new){
  unsigned int i;
  for(i = 0; i < cs->n_constants; i++){
    constant c = cs->constants[i];
    if(strcmp(c.name, new) == 0)
      return i;
  }
  return insert_constant_name(cs, new);
}

/**
   Prints all constants, with partitions
**/
void print_all_constants(constants* cs, FILE* stream){
  unsigned int partitions[cs->n_constants][cs->n_constants];
  unsigned int size_partition[cs->n_constants];
  unsigned int i;
  for(i = 0; i < cs->n_constants; i++)
    size_partition[i] = 0;
  for(i = 0; i < cs->n_constants; i++){
    unsigned int p = find_constant_root(i, cs);
    partitions[p][size_partition[p]] = i;
    size_partition[p]++;
  }
  for(i = 0; i < cs->n_constants; i++){
    if(size_partition[i] > 0){
      unsigned int j;
      fprintf(stream, "{");
      for(j = 0; j < size_partition[i]; j++)
	fprintf(stream, "%s ", cs->constants[partitions[i][j]].name);
      fprintf(stream, "}");
    }
  }
  fprintf(stream, "\n");
}


void print_coq_constants(const constants* cs, FILE* stream){
  unsigned int i;
  if(cs->n_constants > 0){
    fprintf(stream, "Variables ");
    for(i = 0; i < cs->n_constants; i++){
      char first = cs->constants[i].name[0];
      if(first - '0' >= 0 && first - '0' <= 9)
	fprintf(stream, "num_%s ", cs->constants[i].name);
      else
	fprintf(stream, "%s ", cs->constants[i].name);
    }
    fprintf(stream, ": %s.\n", DOMAIN_SET_NAME);
  }
}
