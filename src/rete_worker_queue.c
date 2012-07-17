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
void lock_worker_queue(rete_worker_queue* rq, const char* filename, int line){
  pt_err(pthread_mutex_lock(& rq->queue_mutex), filename, line, "rete_worker_queue.c: lock_worker_queue: mutex_lock");
}

void unlock_worker_queue(rete_worker_queue* rq, const char* filename, int line){
  pt_err(pthread_mutex_unlock(& rq->queue_mutex), filename, line,  "rete_worker_queue.c: unlock_worker_queue: mutex_unlock");
}

void wait_worker_queue(rete_worker_queue* wq, const char* filename, int line){
  pt_err(pthread_cond_wait(& wq->queue_cond, & wq->queue_mutex), filename, line,  "rete_worker_queue.c: wait_worker_queue: cond wait");
}


void signal_worker_queue(rete_worker_queue* rq, const char* filename, int line){
  pt_err(pthread_cond_signal(& rq->queue_cond), filename, line,  "rete_worker_queue.c: signal_worker_queue: cond signal");
}



void broadcast_worker_queue(rete_worker_queue* rq, const char* filename, int line){
  pt_err(pthread_cond_broadcast(& rq->queue_cond), filename, line,  "rete_worker_queue.c: broadcast_worker_queue: cond wait");
}

#endif

size_t get_worker_queue_size_t(size_t size_queue){
  return size_queue * sizeof(worker_queue_elem);
}

void resize_worker_queue(rete_worker_queue* rq){
  rq->queue = realloc_tester(rq->queue, get_worker_queue_size_t(rq->size_queue));
}

void check_worker_queue_too_large(rete_worker_queue* rq){
  while(rq->end < rq->size_queue / 2 && rq->size_queue > 20){
    rq->size_queue /= 2;
    resize_worker_queue(rq);
  }
}

void check_worker_queue_too_small(rete_worker_queue* rq){
  if(rq->end + 1 >= rq->size_queue){
    rq->size_queue *= 2;
    resize_worker_queue(rq);
  }
}



/**
   Creates the initial queue containing
   all facts from the theory

   Also allocates memory for the qeue
 **/
rete_worker_queue* init_rete_worker_queue(void){
  rete_worker_queue* rq = malloc_tester(sizeof(rete_worker_queue));
  rq->size_queue = WORKER_QUEUE_INIT_SIZE;
  rq->queue = calloc_tester(rq->size_queue, sizeof(worker_queue_elem));
#ifdef HAVE_PTHREAD
  pthread_mutexattr_t mutex_attr;
#endif
  rq->first = 0;
  rq->end = 0;

#ifdef HAVE_PTHREAD
  pt_err(pthread_mutexattr_init(&mutex_attr),__FILE__, __LINE__, "rete_worker_queue.c: initialize_queue_single: mutex attr init");
#ifndef NDEBUG
  pt_err(pthread_mutexattr_settype(&mutex_attr, PTHREAD_MUTEX_ERRORCHECK_NP), __FILE__, __LINE__,  "rete_worker_queue.c: initialize_queue_single: mutex attr settype");
#endif
  pt_err(pthread_mutex_init(& rq->queue_mutex, & mutex_attr), __FILE__, __LINE__,   "rete_worker_queue.c: initialize_queue_single: mutex init");
  pt_err(pthread_mutexattr_destroy(&mutex_attr), __FILE__, __LINE__,   "rete_worker_queue.c: initialize_queue_single: mutexattr destroy");
  pt_err(pthread_cond_init(& rq->queue_cond, NULL), __FILE__, __LINE__,   "rete_worker_queue.c: initialize_queue_single: cond init");
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
  pt_err( pthread_mutex_destroy(&rq->queue_mutex),__FILE__, __LINE__,  "rete_worker_queue.c: destroy_rete_worker_queue: mutex destroy");
  pt_err( pthread_cond_destroy(&rq->queue_cond),__FILE__, __LINE__,   "rete_worker_queue.c: destroy_rete_worker_queue: cond destroy");
  
#endif
  free(rq->queue);
  free(rq);
}

/**
   Assigns position pos in the rule queue the according values.
**/
void assign_worker_queue_elem(rete_worker_queue* rq, unsigned int pos, const clp_atom* fact, const rete_node* alpha, unsigned int step){
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
void push_rete_worker_queue(rete_worker_queue * rq, const clp_atom* fact, const rete_node * alpha, unsigned int step){
  unsigned int pos;
#ifdef HAVE_PTHREAD
  lock_worker_queue(rq, __FILE__, __LINE__ );
#endif
  check_worker_queue_too_small(rq);
  pos = rq->end;
  rq->end ++;
  assign_worker_queue_elem(rq, pos, fact, alpha, step);
#ifdef HAVE_PTHREAD
  signal_worker_queue(rq, __FILE__, __LINE__ );
  unlock_worker_queue(rq, __FILE__, __LINE__ );
#endif
}

unsigned int get_timestamp_rete_worker_queue(rete_worker_queue* rq){
  assert(!rete_worker_queue_is_empty(rq));
  return rq->queue[rq->first].step;
}

bool rete_worker_queue_is_empty(rete_worker_queue* rq){
  bool is_empty;
  is_empty = rq->first == rq->end;
  return is_empty;
}

/**
   Returns a pointer to the position of a rule_instance

   Note that this pointer may be invalidated on the next call to insert_.. or pop_...
   and the data maybe destroyed on the first backtracking
**/
void  pop_rete_worker_queue(rete_worker_queue* rq, const clp_atom** fact, const rete_node** alpha, unsigned int* step){
  worker_queue_elem* el = get_worker_queue_elem(rq, rq->first);
  *fact = el->fact;
  *alpha = el->alpha;
  *step = el->step;
  assert(rq->end > rq->first);
  __sync_add_and_fetch(& rq->first, 1);
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
  __sync_lock_test_and_set(& rq->first, backup->first);
  __sync_lock_test_and_set(& rq->end, backup->end);
  check_worker_queue_too_large(rq);
  return rq;
}



/**
   Printing
**/
void print_rete_worker_queue(rete_worker_queue* rq, const constants* cs, FILE* f){
  unsigned int i, j;
  fprintf(f, "queue with %u entries: \n", get_rete_worker_queue_size(rq));
  for(j=0, i = rq->first; i < rq->end; i++, j++){
    worker_queue_elem* el = get_worker_queue_elem(rq, i);
    fprintf(f, "\t%i: ", j);
    print_fol_atom(el->fact, cs, f);
    print_rete_node(el->alpha, cs, f,0);    
    fprintf(f, "\n");
  }
}
    
