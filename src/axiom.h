/* axiom.h

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

#ifndef __INCLUDE_AXIOM_H
#define __INCLUDE_AXIOM_H

#include "common.h"
#include "disjunction.h"
#include "conjunction.h"

/**
   The axioms/rules/formulas are an implication
   where the left-hand side is a conjunction of atoms, 
   and the right-hand side is a disjunction of exist. closed
   conjunctions

   axiom_no is an index into an array of 
   pointers to the rule instance queue kept in the rete net

   exist_vars is the variables that are bound existentially on the right hand side

   a fact has empty premiss, but may have a disjunction in the right hand side
 **/
enum axiom_type { fact, goal, normal };


typedef struct axiom_t {
  enum axiom_type type;
  int axiom_no;
  const clp_conjunction * lhs;
  const clp_disjunction * rhs;
  const char* name;
  bool has_name;
  bool is_existential;
  freevars* exist_vars;
} clp_axiom;


clp_axiom* create_goal(clp_conjunction*);
clp_axiom* create_negative(clp_conjunction*);

void set_axiom_name(clp_axiom*, const char*);

bool test_axiom(const clp_axiom*, size_t, const constants*);

void delete_axiom(clp_axiom*);

freevars* free_axiom_variables(const clp_axiom*, freevars*);

void print_fol_axiom(const clp_axiom*, const constants*, FILE*);
void print_dot_axiom(const clp_axiom*, const constants*, FILE*);
void print_coq_axiom(const clp_axiom*, const constants*, FILE*);

void print_geolog_axiom(const clp_axiom*, const constants*, FILE*);
void print_clpl_axiom(const clp_axiom*, const constants*, FILE*);
void print_tptp_axiom(const clp_axiom* , const constants*, FILE *);

bool is_definite(clp_axiom*);
bool is_existential(clp_axiom*);
bool is_fact(const clp_axiom*);

#endif
