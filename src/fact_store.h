/* fact_store.h

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

#ifndef __INCLUDED_FACT_STORE_H
#define __INCLUDED_FACT_STORE_H

#define INIT_FACT_STORE_SIZE 1000

#include "common.h"
#include "atom.h"

/**
   A store of facts for use in state factsets

   Not thread-safe, since the assumption is that these are
   only accessed from a single rete state.
   
   Uses information calculated in fact_memory.c about
   size needed for facts
**/

/**
   Implements a cache of facts in a alpha or beta node
**/


typedef struct fact_store_t {
  clp_atom * store; 
  unsigned int max_n_facts;
  unsigned int n_facts;
} fact_store;

typedef struct fact_store_backup_t {
  unsigned int n_facts;
  fact_store* store;
} fact_store_backup;

typedef struct fact_store_iter_t {
  unsigned int n;
  fact_store* store;
} fact_store_iter;

fact_store init_fact_store(void);
void destroy_fact_store(fact_store*);
unsigned int alloc_store_fact(fact_store*);
void push_fact_store(fact_store*, const clp_atom*);
const clp_atom* get_fact(unsigned int, fact_store*);
bool is_empty_fact_store(fact_store*);

fact_store_iter get_fact_store_iter(fact_store*);
bool has_next_fact_store(fact_store_iter*);
const clp_atom* get_next_fact_store(fact_store_iter*);
void destroy_fact_store_iter(fact_store_iter*);


fact_store_backup backup_fact_store(fact_store*);
void restore_fact_store(fact_store*, fact_store_backup);
void destroy_fact_backup(fact_store_backup*);

fact_store_backup * backup_fact_store_array(fact_store*, unsigned int);
void destroy_fact_store_backup_array(fact_store_backup*, unsigned int);
void copy_fact_iter_array(fact_store_iter*, const fact_store_iter *, unsigned int);

void print_fact_store(fact_store *, const constants*, FILE*);
#endif
