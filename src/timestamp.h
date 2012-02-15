/* timestamp.h

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

#ifndef __INCLUDED_TIMESTAMP_H
#define __INCLUDED_TIMESTAMP_H

#include "common.h"

/**
   Used to keep info about the steps that were necessary to infer a fact
**/

typedef enum timestamp_type_t { normal_timestamp, equality_timestamp, domain_timestamp } 
  timestamp_type;

typedef enum equality_tactic_t { reflexivity_tactic, rewrite_tactic, inv_rewrite_tactic }
  equality_tactic;

typedef struct timestamp_t {
  timestamp_type type;
  signed int step;
  bool init_model;
  equality_tactic tactic;
} timestamp;
  


timestamp create_normal_timestamp(unsigned int);
bool is_normal_timestamp(timestamp);
bool is_equality_timestamp(timestamp);
bool is_inv_rewrite_timestamp(timestamp);
bool is_reflexivity_timestamp(timestamp);
int compare_timestamp(timestamp, timestamp);


#endif
