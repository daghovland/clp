/* fact_set.h

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

#ifndef __INCLUDE_FACT_SET_H
#define __INCLUDE_FACT_SET_H
/**
   Representation of the fact set as a linked list of atoms
**/
typedef struct fact_set_t {
  struct fact_set_t * next;
  bool split_point;
  const atom* fact;
} fact_set;


fact_set* create_fact_set(void);

void split_fact_set(fact_set*);

void delete_fact_set(fact_set*);

fact_set* insert_fact_set(fact_set*, const atom*);

fact_set* print_fact_set(fact_set*, FILE*);
bool is_in_fact_set(const fact_set*, const atom*);

#endif
