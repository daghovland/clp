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
  const conjunction * lhs;
  const disjunction * rhs;
  const char* name;
  bool has_name;
  bool is_existential;
  freevars* exist_vars;
} axiom;


axiom* create_axiom(conjunction*, disjunction*);
axiom* create_goal(conjunction*);
axiom* create_negative(conjunction*);

void set_axiom_name(axiom*, const char*);

bool test_axiom(const axiom*, size_t);

void delete_axiom(axiom*);

freevars* free_axiom_variables(const axiom*, freevars*);

void print_fol_axiom(const axiom*, const constants*, FILE*);
void print_dot_axiom(const axiom*, const constants*, FILE*);
void print_coq_axiom(const axiom*, const constants*, FILE*);

void print_geolog_axiom(const axiom*, const constants*, FILE*);

bool is_definite(axiom*);
bool is_existential(axiom*);

#endif
