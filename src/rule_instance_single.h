/* rule_instance_single.h

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
#ifndef __INCLUDED_RULE_INSTANCE_SINGLE_H
#define __INCLUDED_RULE_INSTANCE_SINGLE_H

#include "substitution_struct.h"
#include "axiom.h"

typedef struct rule_instance_single_t {
  unsigned int timestamp;
  const axiom * rule;
  bool used_in_proof;
  substitution substitution;
  unsigned int sub_ts[];
} rule_instance_single;


#endif
