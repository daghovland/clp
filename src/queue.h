/* queue.h

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
#ifndef __INCLUDED_QUEUE_H
#define __INCLUDED_QUEUE_H

#include "common.h"
#ifdef HAVE_PTHREAD
#include "pthread.h"
#endif

/**
   A generic queue. Plan is to use for queues of rule_instances and
   pairs of substitutions, facts and alpha nodes
**/
typedef struct queue_t {
#ifdef HAVE_PTHREAD
  pthread_mutex_t queue_mutex;
  pthread_cond_t queue_cond;
#endif
  unsigned int size_queue;
  unsigned int first;
  unsigned int end;
  unsigned int previous_appl;
  unsigned int n_appl;
  unsigned int size_queue_elem;
  char queue[];
} queue;


typedef struct queue_backup_t {
  unsigned int first;
  unsigned int end;
  unsigned int previous_appl;
  unsigned int n_appl;
} queue_backup;

queue* initialize_queue(unsigned int);
void destroy_queue(queue*);

char* push_queue(queue**, const axiom*, const substitution*, unsigned int, bool);
char* pop_queue(queue**, unsigned int);
char* peek_queue(queue*);
char* get_queue_elem(queue*, unsigned int);

unsigned int queue_age(queue*);
unsigned int queue_previous_application(queue*);

queue_backup backup_queue(queue*);
queue* restore_queue(queue*, queue_backup*);

bool queue_is_empty(queue*);
unsigned int get_queue_size(const queue*);

void print_queue(queue*, FILE*);
#endif
