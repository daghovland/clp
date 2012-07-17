/* rule_queue_single.h

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
#ifndef __INCLUDED_RULE_QUEUE_SINGLE_H
#define __INCLUDED_RULE_QUEUE_SINGLE_H

#include "rule_instance.h"
#include "substitution_size_info.h"
#ifdef HAVE_PTHREAD
#include <pthread.h>
#endif

#define RULE_QUEUE_INIT_SIZE 10
/**
   The queue of rule instances to be treated
   Called "conflict set" by Forgy

   Is a fifo, implemented by an array

   first is the index of the oldest element, while end is the next free index

   previous_appl is used by the strategy, and indicates the
   timestamp of the previous time instances were removed from the queue. 
   n_appl indicates how many times instances have been removed from the queue.

   permanent is true for the queue used for the history in rete_state_single. 
   It leads to the timestamps not being freed on backtracking
**/
typedef struct rule_queue_single_t {
#ifdef HAVE_PTHREAD
  pthread_mutex_t queue_mutex;
  pthread_cond_t queue_cond;
#endif
  char * queue;
  bool permanent;
  size_t size_queue;
  size_t first;
  size_t end;
  unsigned int axiom_no;
  bool multi_rule_queue;
  unsigned int previous_appl;
  unsigned int n_appl;
  substitution_size_info ssi;
} rule_queue_single;


typedef struct rule_queue_single_backup_t {
  unsigned int first;
  unsigned int end;
  unsigned int previous_appl;
  unsigned int n_appl;
} rule_queue_single_backup;

rule_queue_single* initialize_queue_single(substitution_size_info, unsigned int, bool permanent, bool multi_rule_queue);
void destroy_rule_queue_single(rule_queue_single*);

rule_instance* push_rule_instance_single(rule_queue_single*, const clp_axiom*, const substitution*, unsigned int, bool, timestamp_store*, const constants*);
rule_instance* pop_rule_queue_single(rule_queue_single*, unsigned int, const constants*);
rule_instance* peek_rule_queue_single(rule_queue_single*, const constants*);
rule_instance* get_rule_instance_single(rule_queue_single*, unsigned int);


void wait_queue_single(rule_queue_single*, const char*, int);
bool timedwait_queue_single(rule_queue_single*, unsigned int, const char*, int);
void signal_queue_single(rule_queue_single*, const char*, int);
void lock_queue_single(rule_queue_single*, const char*, int);
void unlock_queue_single(rule_queue_single*, const char*, int);


unsigned int rule_queue_single_age(rule_queue_single*, const constants*);
unsigned int rule_queue_single_previous_application(rule_queue_single*);

rule_queue_single_backup backup_rule_queue_single(rule_queue_single*);
rule_queue_single* restore_rule_queue_single(rule_queue_single*, rule_queue_single_backup*);

bool rule_queue_single_is_empty(rule_queue_single*);
unsigned int get_rule_queue_single_size(const rule_queue_single*);

bool test_rule_queue_single(rule_queue_single*, const constants*);

void print_rule_queue_single(rule_queue_single*, const constants*, FILE*);
#endif
