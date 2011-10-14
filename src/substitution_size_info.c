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
#include "rule_instance.h"
#include "term.h"

/**
   Returns the size necessary for a substituton structure including
   large enough sub_values and timestamps
**/
unsigned int get_size_substitution(substitution_size_info ssi){
  return ssi.size_substitution;
}


unsigned int get_size_rule_instance(substitution_size_info ssi){
  return ssi.size_rule_instance;
}

/**
   Returns the maximum number of timestamps necessary.
   This becomes the size of the array in timestamps
**/
unsigned int get_max_n_timestamps(substitution_size_info ssi){
  return ssi.max_n_timestamps;
}

/**
   Returns the value that must be added to the address to access the sub_values
   component of substitution
**/
unsigned int get_sub_values_offset(substitution_size_info ssi){
  return ssi.sub_values_offset;
}

/**
   Discovers how much space is needed for the timestmaps, substitutions and rule instances
**/
substitution_size_info init_sub_size_info(unsigned int n_vars, unsigned int max_lhs_conjuncts){
  substitution_size_info ssi;
  unsigned int size_vars, size_timestamps;
  ssi.max_n_timestamps = max_lhs_conjuncts;
  size_vars = n_vars * sizeof(term*);
  size_timestamps = ssi.max_n_timestamps * sizeof(signed int);
  assert(sizeof(timestamps) % sizeof(term*) == (size_timestamps + sizeof(timestamps)) % sizeof(term*));
  ssi.size_substitution = sizeof(substitution) + size_vars + size_timestamps;
  ssi.size_rule_instance = sizeof(rule_instance) + size_vars + size_timestamps;
  ssi.sub_values_offset =  ssi.max_n_timestamps;
  return ssi;
}




