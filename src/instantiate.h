/* instantiate.h

   Copyright 2011 ?

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
#ifndef __INCLUDED_INSTANTIATE_H
#define __INCLUDED_INSTANTIATE_H
/**
   Different utility functions relating to substitutions as instantiations of formulas
**/

#include "common.h"
#include "conjunction.h"
#include "substitution_struct.h"
#include "term.h"
#include "atom.h"
#include "constants.h"



/**
   Tests that a substitution has a value for all variables in the conjunction
**/
bool test_is_conj_instantiation(const conjunction* a, const substitution* sub);
const term* instantiate_term(const term* orig, const substitution* sub);
void delete_instantiated_term(const term* orig, term* copy);
const term_list* instantiate_term_list(const term_list* orig, const substitution* sub);
void delete_instantiated_term_list(const term_list* orig, term_list* copy);
bool test_is_atom_instantiation(const atom* a, const substitution* sub);
atom* instantiate_atom(const atom* orig, const substitution* sub);
void delete_instantiated_atom(const atom* orig, atom* copy);
const term* get_fresh_constant(variable* var, constants* constants);
void fresh_exist_constants(const conjunction* con, substitution* sub, constants* constants);
bool find_instantiate_sub(const atom* at, const atom* fact, substitution* sub);

#endif