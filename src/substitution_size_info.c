/* substitution_size_info.c

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

#include "substitution_size_info.h"
#include "substitution_struct.h"
#include "term.h"

unsigned int get_size_substitution(substitution_size_info ssi){
  return ssi->size_substitution;
}

unsigned int get_size_full_substitution(substitution_size_info ssi){
  return ssi->size_full_substitution;
}
unsigned int get_size_timestamps(substitution_size_info ssi){
  ssi->size_timestamps;
}
unsigned int get_substitution_timestamp_offset(substitution_size_info ssi){
  return ssl->substitution_timestamp_offset;
}

substitution_size_info init_sub_size_info(unsigned int n_vars, unsigned int max_lhs_conjuncts){
  substitution_size_info ssi;
  ssi->size_substitution = sizeof(substitution) + n_vars * sizeof(term*) ;
  ssi->size_timestamps = max_lhs_conjuncts;
  ssi->substitution_timestamp_offset = ssi->size_substitution % sizeof(signed int);
  if(ssi->substitution_timestamp_offset != 0)
    ssi->substitution_timestamp_offset = sizeof(signed int) - ssi->substitution_timestamp_offset;
  assert(ssi->substitution_timestamp_offset >= 0 && ssi->substitution_timestamp_offset < sizeof(signed int));
  ssi->size_full_substitution = ssi->size_substitution + ssi->size_timestamps * sizeof(signed int) + ssi->substitution_timestamp_offset;
  return ssi;
}



#endif
