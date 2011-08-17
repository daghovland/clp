/* disjunction.h

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

#ifndef __INCLUDE_DISJUNCTION_H
#define __INCLUDE_DISJUNCTION_H

#include "conjunction.h"

typedef struct disjunction_t {
  conjunction **args;
  bool has_domain_pred;
  size_t n_args;
  size_t size_args;
  freevars *free_vars;
} disjunction;


disjunction* create_disjunction(conjunction*);
disjunction* create_empty_disjunction(void);
disjunction* extend_disjunction(disjunction*, conjunction*);

bool test_disjunction(const disjunction*);

void delete_disjunction(disjunction*);

void print_fol_disj(const disjunction*, FILE*);
void print_dot_disj(const disjunction*, FILE*);
void print_coq_disj(const disjunction*, FILE*);

void print_geolog_disj(const disjunction*, FILE*);

#endif
