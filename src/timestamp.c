/* timestamp.c

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
/**
   timestamps are used in substitutions, to keep
   track of the steps at which the left-hand side were first inferred.
**/
#include "common.h"
#include "timestamp.h"

timestamp create_normal_timestamp(unsigned int step){
  timestamp retval;
  retval.type = normal_timestamp;
  retval.init_model = false;
  retval.step = step;
  return retval;
}

bool is_normal_timestamp(timestamp ts){
  return ts.type == normal_timestamp && ts.step > 0;
}
bool is_equality_timestamp(timestamp ts){
  return ts.type == equality_timestamp;
}
bool is_inv_rewrite_timestamp(timestamp ts){
  return ts.type == equality_timestamp && ts.tactic == inv_rewrite_tactic;
}
bool is_reflexivity_timestamp(timestamp ts){
  return ts.type == equality_timestamp && ts.tactic == reflexivity_tactic;
}


bool is_init_timestamp(timestamp ts){
  return ts.init_model || ts.step == 0;
}


int compare_timestamp(timestamp ts1, timestamp ts2){
  return ts1.step - ts2.step;
}

