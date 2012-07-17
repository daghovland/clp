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
  const clp_predicate* pred;
  const term_list * args;
} clp_atom;



clp_atom* prover_create_atom(const clp_predicate*, const term_list*);


void delete_atom(clp_atom*);
freevars* free_atom_variables(const clp_atom*, freevars*);

bool test_atom(const clp_atom*, const constants*);
bool test_ground_atom(const clp_atom*, const constants*);

void print_fol_atom(const clp_atom*, const constants*, FILE*);
void print_coq_atom(const clp_atom*, const constants*, FILE*);
void print_geolog_atom(const clp_atom*, const constants*, FILE*);

bool equal_atoms(const clp_atom*, const clp_atom*, constants*, timestamps* ts, timestamp_store* store, bool update_ts);

#endif
