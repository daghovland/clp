/* fact_set.c

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

#include "common.h"
#include "atom.h"
#include "fact_set.h"
#include "conjunction.h"
#include "substitution.h"

fact_set* create_fact_set(){
  return NULL;
}

void delete_fact_set(fact_set* f){
  while(f != NULL){
    fact_set* n = f->next;
    free(f);
    f = n;
  }
}

void delete_fact_set_below(fact_set* f, fact_set* limit){
  while(f != NULL && f != limit){
    fact_set* n = f->next;
    free(f);
    f = n;
  }
}


void split_fact_set(fact_set* f){
  if(f != NULL)
    f->split_point = true;
}

/**
   Inserts a fact into the fact set

   Returns true iff the fact is inserted (Not seen before)
**/

bool insert_state_fact_set(fact_set ** f, const clp_atom* a, unsigned int step, constants* cs){
  unsigned int pred_no = a->pred->pred_no;
  fact_set * new_fs = insert_in_fact_set(f[pred_no], a, step, cs);
  if(new_fs != f[pred_no]){
    f[pred_no] = new_fs;
    return true;
  }
  return false; 
}


fact_set* insert_in_fact_set(fact_set* f, const clp_atom* a, unsigned int ts, constants* cs){
  fact_set* new;
  if(is_in_fact_set(f, a, cs))
    return f;
  new = malloc_tester(sizeof(fact_set));
  new->next = f;
  new->split_point = false;
  new->fact = a;
  new->timestamp = ts;
  return new;
}



/**
   Prints all facts currently in the "factset"
**/
void print_state_fact_set(fact_set ** fs, const constants* cs, FILE* stream, unsigned int n_predicates){
  unsigned int i;
  fprintf(stream, "{");

  for(i = 0; i < n_predicates; i++){
    if(fs[i] != NULL)
      print_fact_set(fs[i], cs, stream);
  }
  fprintf(stream, "}\n");
}


void print_fact_set(fact_set* fs, const constants* cs,  FILE* f){
  while(fs != NULL){
    print_fol_atom(fs->fact, cs, f);
    if(fs->next != NULL)
      fprintf(f, ", ");
    fs = fs->next;
  }
}


/**
   Used by unit testing to test that an inserted fact is not already in the factset
   Note that facts are per definition ground
**/
bool is_in_fact_set(const fact_set* fs, const clp_atom* fact, constants* cs){
  while(fs != NULL){
    assert(fact->pred->pred_no == fs->fact->pred->pred_no);
    if(equal_atoms(fs->fact, fact, cs, NULL, NULL, false))
      return true;
    fs = fs->next;
  }
  return false;
}

/**
   Strating implementation of checking the fact set
   This is not finished, just skeletons
**/

bool atom_true_in_fact_set(const fact_set* fs, const clp_atom* a, substitution* sub, constants* cs){
  while(fs != NULL){
    assert(a->pred->pred_no == fs->fact->pred->pred_no);
    if(equal_atoms(fs->fact, a, cs, NULL, NULL, false)){
      return true;
    }
    fs = fs->next;
  }
  return false;
}

unsigned int get_fact_set_timestamp(const fact_set* fs){
  return fs->timestamp;
}


