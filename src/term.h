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
#include "constants_struct.h"

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
  union {
    const char* function;
    clp_variable* var;
    dom_elem  constant;
  } val;
  enum term_type type;
  const struct term_list_t * args;
};



typedef struct term_t clp_term;
typedef struct term_list_t term_list;


clp_term* create_function_term(const char*, const term_list*);
clp_term* prover_create_constant_term(dom_elem);

clp_term* copy_term(const clp_term*);
term_list* copy_term_list(const term_list*);

term_list* create_term_list(const clp_term*);
term_list* extend_term_list(term_list*, const clp_term*);

bool test_term(const clp_term*, const constants*);
bool test_term_list(const term_list*, const constants*);
bool test_ground_term_list(const term_list* tl, const constants*);

void delete_term_list(term_list*);
void delete_term(clp_term*);

freevars* free_term_variables(const clp_term*, freevars*);
freevars* free_term_list_variables(const term_list*, freevars*);

void print_fol_term(const clp_term*, const constants*, FILE*);
void print_coq_term(const clp_term*, const constants*, FILE*);
void print_fol_term_list(const term_list*, const constants*, FILE*);
void print_coq_term_list(const term_list*, const constants*, FILE*);

void print_geolog_term(const clp_term*, const constants*, FILE*);
void print_geolog_term_list(const term_list*, const constants*, FILE*);

bool equal_terms(const clp_term*, const clp_term*, constants*, timestamps*, timestamp_store*, bool update_ts);
bool literally_equal_terms(const clp_term*, const clp_term*, constants*, timestamps*, timestamp_store*, bool update_ts);
#endif
