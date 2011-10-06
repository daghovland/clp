/* substitution_size_info.h

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

#ifndef __INCLUDE_SUBSTITUTION_SIZE_INFO_H
#define __INCLUDE_SUBSTITUTION_SIZE_INFO_H

/**
   This structure is in the rete net and contains info about
   the size usage of substitutions and the components.
   
   Used by substitution_memory
**/

typedef struct substitution_size_info_t {
  unsigned int size_substitution;
  unsigned int size_timestamps;
  unsigned int size_full_substitution;
  int substitution_timestamp_offset;
} substitution_size_info;

substitution_size_info init_sub_size_info(unsigned int n_vars, unsigned int max_lhs_conjuncts);
unsigned int get_size_substitution(substitution_size_info);
unsigned int get_size_full_substitution(substitution_size_info);
unsigned int get_size_timestamps(substitution_size_info);
unsigned int get_substitution_timestamp_offset(substitution_size_info);


#endif
