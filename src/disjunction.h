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
  clp_conjunction **args;
  size_t n_args;
  size_t size_args;
  freevars *free_vars;
} clp_disjunction;


clp_disjunction* create_disjunction(clp_conjunction*);
clp_disjunction* create_empty_disjunction(void);
clp_disjunction* extend_disjunction(clp_disjunction*, clp_conjunction*);

bool test_disjunction(const clp_disjunction*, const constants*);

void delete_disjunction(clp_disjunction*);

void print_fol_disj(const clp_disjunction*, const constants*, FILE*);
void print_dot_disj(const clp_disjunction*, const constants*, FILE*);
void print_coq_disj(const clp_disjunction*, const constants*, FILE*);

void print_clpl_disj(const clp_disjunction*, const constants*, FILE*);
void print_geolog_disj(const clp_disjunction*, const constants*, FILE*);
void print_tptp_disj(const clp_disjunction*, const constants*, FILE*);

#endif
