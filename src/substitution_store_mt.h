/* substitution_store_mt.h

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

#ifndef __INCLUDE_SUBSTITUTION_STORE_MT_H
#define __INCLUDE_SUBSTITUTION_STORE_MT_H

#include "substitution_struct.h"
#include "substitution_size_info.h"

#define INIT_SUBST_MEM_SIZE 10
/**
   A thread-safe (multi-threaded mt) store of substitutions

**/
typedef struct substitution_store_mt_t {
  char** substitution_stores;
  size_t size_substitution_store;
  size_t * n_cur_substitution_store;
  size_t n_substitution_stores;
  size_t size_substitution_stores;
#ifdef HAVE_PTHREAD
  pthread_mutex_t sub_store_mutex;
  pthread_cond_t sub_store_cond;
#endif
} substitution_store_mt;


substitution_store_mt init_substitution_store_mt(substitution_size_info);
void destroy_substitution_store_mt(substitution_store_mt*);
substitution* get_substitution_store_mt(substitution_store_mt*, substitution_size_info);
void destroy_substitution_store_mt(substitution_store_mt*);
#endif
