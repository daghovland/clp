/* rete_worker_queue.h

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
#ifndef __INCLUDED_RETE_WORKER_QUEUE_H
#define __INCLUDED_RETE_WORKER_QUEUE_H

#include "common.h"
#include "rete_node.h"
#include "atom.h"
#ifdef HAVE_PTHREAD
#include "pthread.h"
#endif

#define WORKER_QUEUE_INIT_SIZE 10

typedef struct worker_queue_elem_t {
  unsigned int step;
  const clp_atom* fact;
  const rete_node * alpha;
} worker_queue_elem;

/**
   The queue of substitutions and rete nodes waiting 
   for a "rete worker" to put into the alpha nodes of a rule

   They are pushed by the prover and popped by the worker thread

   recheck_net is set to true by the prover whenever a new equality is encountered
**/
typedef struct rete_worker_queue_t {
#ifdef HAVE_PTHREAD
  pthread_mutex_t queue_mutex;
  pthread_cond_t queue_cond;
#endif
  worker_queue_elem * queue;
  size_t size_queue;
  size_t first;
  size_t end;
} rete_worker_queue;


typedef struct rete_worker_queue_backup_t {
  unsigned int first;
  unsigned int end;
  unsigned int n_appl;
} rete_worker_queue_backup;

rete_worker_queue* init_rete_worker_queue(void);
void destroy_rete_worker_queue(rete_worker_queue*);


void push_rete_worker_queue(rete_worker_queue*, const clp_atom*, const rete_node*, unsigned int); 
void pop_rete_worker_queue(rete_worker_queue*, const clp_atom**, const rete_node**, unsigned int*);
unsigned int get_timestamp_rete_worker_queue(rete_worker_queue*);
void unpop_rete_worker_queue(rete_worker_queue*);

#ifdef HAVE_PTHREAD
void lock_worker_queue(rete_worker_queue*, const char*, int);
void unlock_worker_queue(rete_worker_queue*, const char*, int);
void signal_worker_queue(rete_worker_queue*, const char*, int);
void broadcast_worker_queue(rete_worker_queue*, const char*, int);
void wait_worker_queue(rete_worker_queue*, const char*, int);
#endif

rete_worker_queue_backup backup_rete_worker_queue(rete_worker_queue*);
rete_worker_queue* restore_rete_worker_queue(rete_worker_queue*, rete_worker_queue_backup*);

bool rete_worker_queue_is_empty(rete_worker_queue*);
unsigned int get_rete_worker_queue_size(const rete_worker_queue*);

void print_rete_worker_queue(rete_worker_queue*, const constants*, FILE*);
#endif
