/* timestamp_array_struct.h

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

/*   Written 2012 by Dag Hovland, hovlanddag@gmail.com  */
/**
   Included from timestamps.h, depending on USE_TIMESTAMP_ARRAY, defined in common.h
**/

#ifndef __INCLUDED_TIMESTAMP_ARRAY_STRUCT_H
#define __INCLUDED_TIMESTAMP_ARRAY_STRUCT_H

#include "common.h"
#include "timestamp.h"


#ifndef USE_TIMESTAMP_ARRAY
#abort
#endif


/**
   Used to keep info about the steps that were necessary to infer a fact
**/


typedef struct timestamps_t {
  unsigned int n_timestamps;
  timestamp timestamps[];
} timestamps;  

typedef struct timestamps_iter_t {
  unsigned int n;
  const timestamps* ts;
} timestamps_iter;

typedef void timestamp_store;
typedef int timestamp_store_backup;

#endif
