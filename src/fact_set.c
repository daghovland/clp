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

fact_set* insert_in_fact_set(fact_set* f, const atom* a){
  fact_set* new;
  if(is_in_fact_set(f, a))
    return f;
  new = malloc_tester(sizeof(fact_set));
  new->next = f;
  new->split_point = false;
  new->fact = a;
  return new;
}

fact_set* print_fact_set(fact_set* fs, FILE* f){
  fprintf(f, "{");
  while(fs != NULL){
    print_fol_atom(fs->fact, f);
    if(fs->next != NULL)
      fprintf(f, ", ");
    fs = fs->next;
  }
  fprintf(f, "}\n");
}


/**
   Used by unit testing to test that an inserted fact is not already in the factset
   Note that facts are per definition ground
**/
bool is_in_fact_set(const fact_set* fs, const atom* fact){
  while(fs != NULL){
    assert(fact->pred->pred_no == fs->fact->pred->pred_no);
    if(equal_atoms(fs->fact, fact))
      return true;
    fs = fs->next;
  }
  return false;
}

/**
   Strating implementation of checking the fact set
   This is not finished, just skeletons
**/

bool atom_true_in_fact_set(const fact_set* fs, const atom* a, substitution* sub){
  while(fs != NULL){
    assert(a->pred->pred_no == fs->fact->pred->pred_no);
    if(equal_atoms(fs->fact, a)){
      return true;
    }
    fs = fs->next;
  }
  return false;
}




