/* atom.h

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

#ifndef __INCLUDE_ATOM_H
#define __INCLUDE_ATOM_H

#include "common.h"
#include "term.h"
#include "predicate.h"


typedef struct atom_t {
  const predicate* pred;
  const term_list * args;
} atom;



atom* prover_create_atom(const predicate*, const term_list*);


void delete_atom(atom*);
freevars* free_atom_variables(const atom*, freevars*);

bool test_atom(const atom*);
bool test_ground_atom(const atom*);

void print_fol_atom(const atom*, const constants*, FILE*);
void print_coq_atom(const atom*, const constants*, FILE*);
void print_geolog_atom(const atom*, const constants*, FILE*);

bool equal_atoms(const atom*, const atom*, constants*);

#endif
