/* fresh_constants.h

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
#ifndef __INCLUDED_FRESH_CONSTANTS
#define __INCLUDED_FRESH_CONSTANTS

#include "common.h"


/**
  This structure is accessed and shared from all threads

  Therefore the atomic variables

  rete_net_state has pointer to this object

  There is no size information, since this is the
  same as the number of variables
**/

typedef unsigned int*  fresh_const_counter;

fresh_const_counter init_fresh_const(size_t num_vars);

/**
   This functions is thread-safe
**/
unsigned int next_fresh_const_no(fresh_const_counter, size_t var_no);

#endif
