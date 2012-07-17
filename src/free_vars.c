/* free_vars.c

   Copyright 2008 Free Software Foundation

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
#include "variable.h"

/** 
    Utility functions for the structure freevars:
    Keeps track of free variables in an expression
**/



freevars* init_freevars(){
  size_t size = 2;
  freevars* ret_val = malloc_tester(sizeof(freevars) + size * sizeof(clp_variable*));
  ret_val->n_vars = 0;
  ret_val->size_vars = size;
  return ret_val;
}

/*
  Sets n_vars to 0, which effectively means
  removing all variables
*/
void reset_freevars(freevars* p){
  p->n_vars = 0;
}

freevars* copy_freevars(const freevars* orig){
  size_t size = sizeof(freevars) + orig->size_vars * sizeof(clp_variable*);
  freevars* copy = malloc_tester(size);
  memcpy(copy, orig, size);
  assert(copy->n_vars == 0 || strncmp(copy->vars[copy->n_vars-1]->name, orig->vars[orig->n_vars-1]->name, strlen(orig->vars[orig->n_vars-1]->name)) == 0);
  return copy;
}

void del_freevars(freevars* fv){  
  assert(fv != NULL);
  free(fv);
}

/*
  Testing whether a freevar set is empty
*/
bool is_empty_free_vars(const freevars* fv){
  return fv->n_vars == 0;
}

/*
  Variables in "add" are added to "orig", which is modified in place
*/
freevars* plus_freevars(freevars* orig, const freevars* add){
  unsigned int i;
  for(i = 0; i < add->n_vars; i++)
    orig = add_freevars(orig, add->vars[i]);
  return orig;
}

freevars* add_freevars(freevars* orig, clp_variable* new){
  int i;
  for(i = 0; i < orig->n_vars; i++){
    if(orig->vars[i]->var_no == new->var_no)
      return orig;
  }
  if(orig->n_vars+1 >= orig->size_vars){
    orig->size_vars *= 2;
    orig = realloc_tester(orig, sizeof(freevars) + (orig->size_vars+1) * sizeof(clp_variable*));
  }
  orig->vars[orig->n_vars] = new;

  assert(orig->vars[orig->n_vars]->var_no == new->var_no);
  
  orig->n_vars++;
  return orig;
}

/**
   Only used by the parser to create the original list of variables

   Identity of variables is based on the string name
**/
clp_variable* parser_new_variable(freevars** vars, const char* new){
  freevars_iter iter = get_freevars_iter(*vars);
  clp_variable * next;
  while(has_next_freevars_iter(&iter)){
    next = next_freevars_iter(&iter);
    if(strcmp(next->name, new) == 0)
      return next;
  }
  next = malloc(sizeof(clp_variable));
  next->name = new;
  next->var_no = (*vars)->n_vars;
  *vars = add_freevars(*vars, next);
  return next;
}


/**
   An iterator for freevars
**/
freevars_iter get_freevars_iter(const freevars* fv){
  freevars_iter retval = { 0, fv };
  return retval;
}

clp_variable* next_freevars_iter(freevars_iter* i){
  assert(i->next < i->vars->n_vars);
  return(i->vars->vars[i->next++]);
}

bool has_next_freevars_iter(const freevars_iter* i){
  return i->next < i->vars->n_vars;
}
/**
   Tests that the intersection of two freevars structures is empty

   Used by test_axiom and remove_freevars
**/
bool empty_intersection(const freevars* fv1, const freevars* fv2){
  unsigned int i, j;
  for(i = 0; i < fv1->n_vars; i++){
    for(j = 0; j < fv2->n_vars; j++){
      if(fv1->vars[i]->var_no == fv2->vars[j]->var_no)
	return false;
    }
  }
  return true;
}

/**
   Tests that fv1 is included in fv2

   Used by insert_beta_not_nodes in con_dis.c
**/
bool freevars_included(const freevars* fv1, const freevars* fv2){
  unsigned int i, j;
  for(i = 0; i < fv1->n_vars; i++){
    bool is_in_fv2 = false;
    for(j = 0; j < fv2->n_vars; j++){
      if(fv1->vars[i]->var_no == fv2->vars[j]->var_no){
	is_in_fv2 = true;
	break;
      }
    }
    if(!is_in_fv2)
      return false;
  }
  return true;
}


/**
   returns true iff the var is in fv
**/
bool is_in_freevars(const freevars* fv, const clp_variable* var){
  unsigned int i;
  for(i = 0; i < fv->n_vars; i++){
    if (var->var_no == fv->vars[i]->var_no)
      return true;
  }
  return false;
}

/**
   Removes the variables occurring in subtract from orig
**/
void remove_freevars(freevars* orig, const freevars* subtract){
  int i, j;
  for(i = 0; i < orig->n_vars; i++){
    for(j = 0; j < subtract->n_vars; j++){
      if(orig->vars[i]->var_no == subtract->vars[j]->var_no){
	orig->n_vars--;
	assert(orig->n_vars >= 0 && i <= orig->n_vars);
	assert(strlen(orig->vars[orig->n_vars]->name) > 0);
	orig->vars[i] = orig->vars[orig->n_vars];
	i--;
	break;
      }
    }
  }
  assert(empty_intersection(orig, subtract));
}

/**
 **/
void print_variable(const clp_variable* var, FILE* f){
  fprintf(f, "%s", var->name);
}
