/* theory.h

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

#ifndef __INCLUDE_THEORY_H
#define __INCLUDE_THEORY_H

#include "common.h"
#include "axiom.h"
#include "term.h"
#include "atom.h"

typedef struct theory_t {
  const axiom** axioms;
  unsigned int n_axioms;
  size_t size_axioms;
  freevars* vars;
  size_t n_predicates;
  size_t size_predicates;
  predicate** predicates;
  size_t n_func_names;
  size_t size_func_names;
  const char** func_names;
  size_t n_constants;
  size_t size_constants;
  const char** constants;
} theory;


theory* create_theory(void);
void extend_theory(theory*, axiom*);

term* create_variable(const char*, theory*);
atom* parser_create_atom(const char*, const term_list*, theory*);
term* parser_create_constant_term(theory*, const char*);
atom* create_prop_variable(const char*, theory*);

bool test_theory(const theory*);

void delete_theory(theory*);
void print_fol_theory(const theory*, FILE*);
void print_geolog_theory(const theory*, FILE*);
const predicate* parser_new_predicate(theory*, const char*, size_t);

#endif
