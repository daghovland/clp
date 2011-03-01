/* term.h

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

#ifndef __INCLUDE_TERM_H
#define __INCLUDE_TERM_H

#include "variable.h"

struct term_t;

struct term_list_t {
  unsigned int n_args;
  size_t size_args;
  const struct term_t ** args;
};

/*
  Terms of type function and constant have a name, 
  while variables have NULL name, but a var
*/


enum term_type { constant_term, variable_term, function_term };

struct term_t {
  const char* name;
  variable* var;
  enum term_type type;
  const struct term_list_t * args;
};



typedef struct term_t term;
typedef struct term_list_t term_list;


term* create_term(const char*, const term_list*);
term* prover_create_constant_term(const char*);

term* copy_term(const term*);
term_list* copy_term_list(const term_list*);

term_list* create_term_list(const term*);
term_list* extend_term_list(term_list*, const term*);

bool test_term(const term*);
bool test_term_list(const term_list*);

void delete_term_list(term_list*);
void delete_term(term*);

void free_term_variables(const term*, freevars*);
void free_term_list_variables(const term_list*, freevars*);

void print_fol_term(const term*, FILE*);
void print_fol_term_list(const term_list*, FILE*);

void print_geolog_term(const term*, FILE*);
void print_geolog_term_list(const term_list*, FILE*);

bool equal_terms(const term*, const term*);

#endif
