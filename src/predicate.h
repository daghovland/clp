/* predicate.h

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

#ifndef __INCLUDE_PREDICATE_H
#define __INCLUDE_PREDICATE_H

#include "common.h"


typedef struct predicate_t {
  const char* name;
  bool is_equality;
  size_t arity;
  size_t pred_no;
} clp_predicate;

bool test_predicate(const clp_predicate*);
void print_coq_predicate(const clp_predicate*, FILE*);

#endif
