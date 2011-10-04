/* substitution_memory.h

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

#ifndef __INCLUDE_SUBSTITUTION_MEMORY_H
#define __INCLUDE_SUBSTITUTION_MEMORY_H

#include "substitution.h"
#include "theory.h"

/**
   A stor of substitutions

**/

void init_substitution_memory(const theory*);
void destroy_substitution_memory(void);
size_t get_size_full_substitution(void);
size_t get_size_substitution(void);
size_t get_size_timestamps(void);
substitution* get_substitution_memory(void);
#endif
