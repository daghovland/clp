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
#include "timestamp.h"

#ifdef USE_TIMESTAMP_ARRAY
#include "timestamp_array_struct.h"
#else
#include "timestamp_linked_list.h"
#endif

#include "substitution_size_info.h"

/**
   Used to keep info about the steps that were necessary to infer a fact
**/



void init_empty_timestamps(timestamps*, substitution_size_info);
unsigned int get_n_timestamps(const timestamps*);

void add_normal_timestamp(timestamps*, unsigned int, timestamp_store*);
void add_equality_timestamp(timestamps*, unsigned int, timestamp_store*, bool);
void add_reflexivity_timestamp(timestamps*, unsigned int, timestamp_store*);
void add_domain_timestamp(timestamps*, unsigned int, timestamp_store*);
void add_timestamp(timestamps*, timestamp, timestamp_store*);
void add_timestamps(timestamps* dest, const timestamps* orig, timestamp_store*);

void copy_timestamps(timestamps* dest, const timestamps* orig, timestamp_store*, bool permanent);
int compare_timestamps(const timestamps*, const timestamps*);
#ifndef NDEBUG
bool test_timestamps(const timestamps*);
#endif
timestamp get_oldest_timestamp(timestamps*);

timestamps_iter get_timestamps_iter(const timestamps*);
bool has_next_timestamps_iter(const timestamps_iter*);
bool has_next_non_eq_timestamps_iter(const timestamps_iter*);
timestamp get_next_timestamps_iter(timestamps_iter*);
void destroy_timestamps_iter(timestamps_iter*);
bool is_init_timestamp(timestamp);

timestamp_store* init_timestamp_store(substitution_size_info);
timestamp_store_backup backup_timestamp_store(timestamp_store*);
timestamp_store* restore_timestamp_store(timestamp_store_backup);
void destroy_timestamp_store(timestamp_store*);
#endif
