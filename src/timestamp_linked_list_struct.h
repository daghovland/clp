/* timestamp_linked_list_struct.h

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

/*   Written 2011-2012 by Dag Hovland, hovlanddag@gmail.com  */
/**
   Included from timestamps.h, depending on USE_TIMESTAMP_ARRAY, defined in common.h
**/
#ifndef __INCLUDED_TIMESTAMP_LINKED_LIST_STRUCT_H
#define __INCLUDED_TIMESTAMP_LINKED_LIST_STRUCT_H

#include "common.h"
#include "timestamp.h"

#ifdef HAVE_PTHREAD
#include <pthread.h>
#endif

#ifdef USE_TIMESTAMP_ARRAY
#abort
#endif


/**
   Used to keep info about the steps that were necessary to infer a fact
   
   A linked list seems necessary when using equality through union-find
**/




typedef struct timestamp_linked_list_t {
  timestamp ts;
  struct timestamp_linked_list_t* next;
} timestamp_linked_list;  

/**
   New elements are added at the "list" position

   n_elems is only calculated when needed
**/
typedef struct timestamps_t {
  timestamp_linked_list* list;
  timestamp_linked_list* first;
} timestamps;

typedef struct timestamps_iter_t {
  const timestamp_linked_list* ts;
} timestamps_iter;

typedef struct timestamp_store_t {
#ifdef HAVE_PTHREAD
  pthread_mutex_t lock;
#endif
  unsigned int size_timestamp_store;
  unsigned int n_timestamp_store;
  unsigned int n_stores;
  unsigned int size_stores;
  timestamp_linked_list **stores;
} timestamp_store;

typedef struct timestamp_store_backup_t {
  timestamp_store* store;
  unsigned int n_timestamp_store;
  unsigned int n_stores;
} timestamp_store_backup;

#endif
