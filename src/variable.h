/* variable.h

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
#ifndef __INCLUDE_VARIABLE_H
#define __INCLUDE_VARIABLE_H

typedef struct variable_t {
  const char* name;
  size_t var_no;
} clp_variable;

typedef struct freevars_t {
  size_t n_vars;
  size_t size_vars;
  clp_variable* vars[];
} freevars;

typedef struct freevars_iter_t {
  size_t next;
  const freevars* vars;
} freevars_iter;

freevars* init_freevars(void);
freevars* add_freevars(freevars*, clp_variable*);
void remove_freevars(freevars*, const freevars*);
int num_freevars(const freevars*);
void del_freevars(freevars*);
freevars* plus_freevars(freevars*, const freevars*);
freevars* copy_freevars(const freevars*);
void reset_freevars(freevars*);
freevars_iter get_freevars_iter(const freevars*);
clp_variable* next_freevars_iter(freevars_iter*);
bool has_next_freevars_iter(const freevars_iter*);
clp_variable* parser_new_variable(freevars**, const char*);
bool empty_intersection(const freevars*, const freevars*);
bool freevars_included(const freevars*, const freevars*);
bool is_in_freevars(const freevars*, const clp_variable*);

bool test_variable(const clp_variable*);
void print_variable(const clp_variable*, FILE*);

#endif
