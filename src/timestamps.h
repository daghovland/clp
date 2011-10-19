/* timestamps.h

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

#ifndef __INCLUDED_TIMESTAMPS_H
#define __INCLUDED_TIMESTAMPS_H

#include "common.h"
#include "substitution_size_info.h"

typedef struct timestamps_t {
  unsigned int n_timestamps;
  signed int timestamps[];
} timestamps;  

typedef struct timestamps_iter_t {
  unsigned int n;
  const timestamps* ts;
} timestamps_iter;

void init_empty_timestamps(timestamps*, substitution_size_info);
unsigned int get_n_timestamps(const timestamps*);

void add_timestamp(timestamps*, unsigned int);
void add_timestamps(timestamps* dest, const timestamps* orig);

int compare_timestamps(const timestamps*, const timestamps*);

timestamps_iter get_timestamps_iter(const timestamps*);
bool has_next_timestamps_iter(const timestamps_iter*);
signed int get_next_timestamps_iter(timestamps_iter*);
void destroy_timestamps_iter(timestamps_iter*);

#endif