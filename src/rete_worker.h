/* rete_worker.h

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
#ifndef __INCLUDED_RETE_WORKER_H
#define __INCLUDED_RETE_WORKER_H

#include "common.h"
#include "rete_worker_queue.h"
#include "rule_queue_single.h"
#include "substitution_store_array.h"
#include "substitution_store_mt.h"
#include "theory.h"
#include "rete_net.h"
#ifdef HAVE_PTHREAD
#include "pthread.h"

/**
   Represents the "worker" that takes care of inserting into the
   part of the rete network coresponding to a single rule. 
   This should run in a separate thread. 
   It is started by rete_state_single.
   It "sleeps" while the rete_worker_queue is empty, pops from this
   and eventually inserts into the rule_queue_single.
   The latter is popped by the prover, and the worker queue is pushed
   in rete_insert_single.c

   The pointers must be double, because both the queues are realloced as a whole (and therefore invalidated.)
**/

typedef struct rete_worker_t {
  pthread_t tid;
  bool working;
  bool stop_worker;
  substitution_store_array * node_subs;
  substitution_store_mt * tmp_subs;
  const rete_net* net;
  rete_worker_queue ** work;
  rule_queue_single ** output;
} rete_worker;

rete_worker* init_rete_worker(const rete_net*, substitution_store_mt *, substitution_store_array *, rule_queue_single **, rete_worker_queue **);
void destroy_rete_worker(rete_worker*);
void restart_rete_worker(rete_worker*);
void pause_rete_worker(rete_worker*);
void continue_rete_worker(rete_worker*);
bool rete_worker_is_working(rete_worker*);

#endif
#endif
