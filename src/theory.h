/* theory.h

   Copyright 2011

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
#include "substitution_size_info.h"


/**
   max_lhs_conjuncts is the maximum number of conjuncts in the left hand side
   of any axiom. This is nice to have for the subsitution timestamp lists, since we
   then know the exact size
**/
typedef struct theory_t {
  const clp_axiom** axioms;
  unsigned int n_axioms;
  size_t size_axioms;
  clp_conjunction ** init_model;
  unsigned int n_init_model;
  size_t size_init_model;
  freevars* vars;
  char* name;
  bool has_name;
  size_t n_predicates;
  size_t size_predicates;
  clp_predicate** predicates;
  size_t n_func_names;
  size_t size_func_names;
  const char** func_names;
  unsigned int max_lhs_conjuncts;
  unsigned int max_rhs_conjuncts;
  unsigned int max_rhs_disjuncts;
  substitution_size_info sub_size_info;
#ifndef NDEBUG
  bool finalized;
#endif
  constants* constants;
} theory;


theory* create_theory(void);
void extend_theory(theory*, clp_axiom*);
void set_theory_name(theory*, const char*);
bool has_theory_name(const theory*);
void finalize_theory(theory*);

void print_coq_proof_intro(const theory*, const constants*, FILE*);
void print_coq_proof_ending(const theory*, FILE*);

/* In atom_and_term.c */
clp_term* create_variable(const char*, theory*);
clp_atom* parser_create_atom(const char*, const term_list*, theory*);
clp_atom* parser_create_equality(const clp_term*, const clp_term*, theory*);
clp_atom* create_dom_atom(const clp_term*, theory*);
clp_term* parser_create_constant_term(theory*, const char*);
clp_atom* create_prop_variable(const char*, theory*);


// In con_dis.c
clp_conjunction* create_empty_conjunction(theory*);
void fix_equality_vars(clp_conjunction*, theory*);


//In axiom.c
clp_axiom* create_fact(clp_disjunction*, theory*);
clp_axiom* create_axiom(clp_conjunction*, clp_disjunction*, theory*);



bool test_theory(const theory*);

void delete_theory(theory*);
void print_fol_theory(const theory*, const constants*, FILE*);
void print_clpl_theory(const theory*, const constants*, FILE*);
void print_geolog_theory(const theory*, const constants*, FILE*);
void print_tptp_theory(const theory*, const constants*, FILE*);
const clp_predicate* parser_new_predicate(theory*, const char*, size_t);

#endif
