/* rete_worker_queue.c

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

#include "common.h"
#include "rete.h"
#include "rete_worker_queue.h"
#include "error_handling.h"


#ifdef HAVE_PTHREAD
/**
   pthread functionality for locking, signaling etc. 
**/
void lock_worker_queue(rete_worker_queue* rq){
  pt_err(pthread_mutex_lock(& rq->queue_mutex), "rete_worker_queue.c: lock_worker_queue: mutex_lock");
}

void unlock_worker_queue(rete_worker_queue* rq){
  pt_err(pthread_mutex_unlock(& rq->queue_mutex), "rete_worker_queue.c: unlock_worker_queue: mutex_unlock");
}

void wait_worker_queue(rete_worker_queue* rq){
  pt_err(pthread_cond_wait(& rq->queue_cond, & rq->queue_mutex), "rete_worker_queue.c: wait_worker_queue: cond wait");
}


void signal_worker_queue(rete_worker_queue* rq){
  pt_err(pthread_cond_signal(& rq->queue_cond), "rete_worker_queue.c: signal_worker_queue: cond signal");
}



void broadcast_worker_queue(rete_worker_queue* rq){
  pt_err(pthread_cond_broadcast(& rq->queue_cond), "rete_worker_queue.c: broadcast_worker_queue: cond wait");
}

#endif

size_t get_worker_queue_size_t(size_t size_queue){
  return sizeof(rete_worker_queue) + size_queue * sizeof(worker_queue_elem);
}

void resize_worker_queue(rete_worker_queue** rq){
  *rq = realloc_tester(*rq, get_worker_queue_size_t((*rq)->size_queue));
}

void check_worker_queue_too_large(rete_worker_queue** rq){
  while((*rq)->end < (*rq)->size_queue / 2 && (*rq)->size_queue > 20){
    (*rq)->size_queue /= 2;
    resize_worker_queue(rq);
  }
}

void check_worker_queue_too_small(rete_worker_queue** rq){
  if((*rq)->end + 1 >= (*rq)->size_queue){
    (*rq)->size_queue *= 2;
    resize_worker_queue(rq);
  }
}

/**
   Creates the initial queue containing
   all facts from the theory

   Also allocates memory for the qeue
 **/
rete_worker_queue* init_rete_worker_queue(void){
  unsigned int i;
  size_t size_queue = 10;
  rete_worker_queue* rq = malloc_tester(get_worker_queue_size_t(size_queue));
#ifdef HAVE_PTHREAD
  pthread_mutexattr_t mutex_attr;
#endif
  rq->first = 0;
  rq->end = 0;
  rq->size_queue = size_queue;
#ifdef HAVE_PTHREAD
  pt_err(pthread_mutexattr_init(&mutex_attr), "rete_worker_queue.c: initialize_queue_single: mutex attr init");
#ifndef NDEBUG
  pt_err(pthread_mutexattr_settype(&mutex_attr, PTHREAD_MUTEX_ERRORCHECK_NP),  "rete_worker_queue.c: initialize_queue_single: mutex attr settype");
#endif
  pt_err(pthread_mutex_init(& rq->queue_mutex, & mutex_attr),   "rete_worker_queue.c: initialize_queue_single: mutex init");
  pt_err(pthread_mutexattr_destroy(&mutex_attr),   "rete_worker_queue.c: initialize_queue_single: mutexattr destroy");
  pt_err(pthread_cond_init(& rq->queue_cond, NULL),   "rete_worker_queue.c: initialize_queue_single: cond init");
#endif
  return rq;
}

unsigned int get_rete_worker_queue_size(const rete_worker_queue* rq){
  return rq->end - rq->first;
}


/**
   Gets a rule instance at a position in the queue
**/
worker_queue_elem* get_worker_queue_elem(rete_worker_queue* rq, unsigned int pos){
  return & rq->queue[pos];
}

const worker_queue_elem* get_const_worker_queue_elem(const rete_worker_queue* rq, unsigned int pos){
  return & rq->queue[pos];
}



/**
   Frees memory allocated for the rule queue
**/
void destroy_rete_worker_queue(rete_worker_queue* rq){
#ifdef HAVE_PTHREAD
  pt_err( pthread_mutex_destroy(&rq->queue_mutex), "rete_worker_queue.c: destroy_rete_worker_queue: mutex destroy");
  pt_err( pthread_cond_destroy(&rq->queue_cond),  "rete_worker_queue.c: destroy_rete_worker_queue: cond destroy");
  
#endif
  free(rq);
}

/**
   Assigns position pos in the rule queue the according values.
**/
void assign_worker_queue_elem(rete_worker_queue* rq, unsigned int pos, const atom* fact, const rete_node* alpha, unsigned int step){
  assert(pos < rq->end);
  worker_queue_elem* el = get_worker_queue_elem(rq, pos);
  el->fact = fact;
  el->alpha = alpha;
  el->step = step;
}

/**
   Inserts a copy of the substitution into a new entry in the rule queue.
   The original substitution is not changed
**/
void push_rete_worker_queue(rete_worker_queue ** rq, const atom* fact, const rete_node * alpha, unsigned int step){
  unsigned int pos;
#ifdef HAVE_PTHREAD
  lock_worker_queue(*rq);
#endif
  check_worker_queue_too_small(rq);
  pos = (*rq)->end;
  (*rq)->end ++;
  assign_worker_queue_elem(*rq, pos, fact, alpha, step);
#ifdef HAVE_PTHREAD
  signal_worker_queue(*rq);
  unlock_worker_queue(*rq);
#endif
}



bool rete_worker_queue_is_empty(rete_worker_queue* rq){
  return rq->first == rq->end;
}

/**
   Returns a pointer to the position of a rule_instance

   Note that this pointer may be invalidated on the next call to insert_.. or pop_...
   and the data maybe destroyed on the first backtracking
**/
void  pop_rete_worker_queue(rete_worker_queue** rqp, const atom** fact, const rete_node** alpha, unsigned int* step){
  rete_worker_queue* rq = *rqp;
  worker_queue_elem* el = get_worker_queue_elem(rq, rq->first);
  *fact = el->fact;
  *alpha = el->alpha;
  *step = el->step;
  assert(rq->end > rq->first);
  rq->first++;
}

/**
   Called after stopping the queue workers, 
   and before backing up
**/
void unpop_rete_worker_queue(rete_worker_queue* rq){
  assert(rq->first > 0);
  rq->first--;
}

rete_worker_queue_backup backup_rete_worker_queue(rete_worker_queue* rq){
  rete_worker_queue_backup backup;
  backup.first = rq->first;
  backup.end = rq->end;
  return backup;
}

rete_worker_queue* restore_rete_worker_queue(rete_worker_queue* rq, rete_worker_queue_backup* backup){
  rq->first = backup->first;
  rq->end = backup->end;
  check_worker_queue_too_large(&rq);
  return rq;
}



/**
   Printing
**/
void print_rete_worker_queue(rete_worker_queue* rq, FILE* f){
  unsigned int i, j;
#ifdef HAVE_PTHREAD
  lock_worker_queue(rq);
#endif
  fprintf(f, "queue with %zi entries: \n", get_rete_worker_queue_size(rq));
  for(j=0, i = rq->first; i < rq->end; i++, j++){
    worker_queue_elem* el = get_worker_queue_elem(rq, i);
    fprintf(f, "\t%i: ", j);
    print_fol_atom(el->fact, f);
    print_rete_node(el->alpha, f);    
    fprintf(f, "\n");
  }
#ifdef HAVE_PTHREAD
  unlock_worker_queue(rq);
#endif
}
    
