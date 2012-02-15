/* timestamps.c

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
   Lists or arrays of timestamps are used in substitutions, to keep
   track of the steps at which the left-hand side were first inferred.

   USE_TIMESTAMP_ARRAY is defined in common.h
**/
#include "common.h"
#include "timestamps.h"
#include "term.h"
#include "substitution.h"
#include "rule_instance.h"


#ifdef USE_TIMESTAMP_ARRAY
#include "timestamp_array.c"
#else 
#include "timestamp_linked_list.c"
#endif


void add_normal_timestamp(timestamps* ts, unsigned int step, timestamp_store* store){
  timestamp t;
  t.type = normal_timestamp;
  t.step = step;
  t.init_model = false;
  add_timestamp(ts, t, store);
}

void add_equality_timestamp(timestamps* ts, unsigned int step, timestamp_store* store, bool normal_rewrite){
  timestamp t;
  t.type = equality_timestamp;
  t.step = step;
  t.init_model = false;
  if(normal_rewrite)
    t.tactic = rewrite_tactic;
  else
    t.tactic = inv_rewrite_tactic;
  add_timestamp(ts, t, store);
}
void add_reflexivity_timestamp(timestamps* ts, unsigned int step, timestamp_store* store){
  timestamp t;
  t.type = equality_timestamp;
  t.step = step;
  t.init_model = false;
  t.tactic = reflexivity_tactic;
  t.tactic = reflexivity_tactic;
  add_timestamp(ts, t, store);
}


void add_domain_timestamp(timestamps* ts, unsigned int step, timestamp_store* store){
  timestamp t;
  t.type = domain_timestamp;
  t.step = step;
  t.init_model = false;
  add_timestamp(ts, t, store);
}

