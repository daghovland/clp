/* conjunction.h

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

#ifndef __INCLUDE_CONJUNCTION_H
#define __INCLUDE_CONJUNCTION_H

#include "common.h"
#include "atom.h" 

typedef struct conjunction_t {
  const atom **args;
  unsigned int n_args;
  size_t size_args;
  bool is_existential;
  unsigned int n_equalities;
  freevars *bound_vars;
  freevars *free_vars;
} conjunction;

conjunction* create_conjunction(const atom*);
conjunction* extend_conjunction(conjunction*, const atom*);

void delete_conjunction(conjunction*);
freevars* free_conj_variables(const conjunction*, freevars*);

bool test_conjunction(const conjunction*, const constants*);

void print_fol_conj(const conjunction*, const constants*, FILE*);
void print_clpl_conj(const conjunction*, const constants*, FILE*);
void print_dot_conj(const conjunction*, const constants*, FILE*);
bool print_coq_conj(const conjunction*, const constants*, FILE*);
void print_geolog_conj(const conjunction*, const constants*, FILE*);
void print_tptp_conj(const conjunction*, const constants*, FILE*);

#endif
