/* queue.c

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
#include "queue.h"
#include "error_handling.h"


#ifdef HAVE_PTHREAD
/**
   pthread functionality for locking, signaling etc. 
**/
void lock_queue(queue* rq){
  pt_err(pthread_mutex_lock(& rq->queue_mutex), "queue.c: lock_queue_single: mutex_lock");
}

void unlock_queue(queue* rq){
  pt_err(pthread_mutex_unlock(& rq->queue_mutex), "queue.c: unlock_queue_single: mutex_unlock");
}
#endif

size_t get_rq_size_t(size_t size_queue, substitution_size_info ssi){
  return sizeof(queue) + size_queue * rq->size_queue_elem;
}

void resize_rq_size(queue** rq){
  *rq = realloc_tester(*rq, get_rq_size_t((*rq)->size_queue, (*rq)->ssi));
}

void check_rq_too_large(queue** rq){
  while((*rq)->end < (*rq)->size_queue / 2 && (*rq)->size_queue > 20){
    (*rq)->size_queue /= 2;
    resize_rq_size(rq);
  }
}

void check_rq_too_small(queue** rq){
  if((*rq)->end + 1 >= (*rq)->size_queue){
    (*rq)->size_queue *= 2;
    resize_rq_size(rq);
  }
}

/**
   Creates the initial queue containing
   all facts from the theory

   Also allocates memory for the qeue
 **/
queue* initialize_queue(unsigned int size_queue_elem){
  unsigned int i;
  size_t size_queue = 10;
  queue* rq = malloc_tester(get_rq_size_t(size_queue, size_queue_elem));
#ifdef HAVE_PTHREAD
  pthread_mutexattr_t mutex_attr;
#endif
  rq->size_queue_elem = size_queue_elem;
  rq->first = 0;
  rq->end = 0;
  rq->previous_appl = 0;
  rq->n_appl = 0;
  rq->size_queue = size_queue;
#ifdef HAVE_PTHREAD
  pt_err(pthread_mutexattr_init(&mutex_attr), "queue.c: initialize_queue_single: mutex attr init");
#ifndef NDEBUG
  pt_err(pthread_mutexattr_settype(&mutex_attr, PTHREAD_MUTEX_ERRORCHECK_NP),  "queue.c: initialize_queue_single: mutex attr settype");
#endif
  pt_err(pthread_mutex_init(& rq->queue_mutex, & mutex_attr),   "queue.c: initialize_queue_single: mutex init");
  pt_err(pthread_mutexattr_destroy(&mutex_attr),   "queue.c: initialize_queue_single: mutexattr destroy");
  pt_err(pthread_cond_init(& rq->queue_cond, NULL),   "queue.c: initialize_queue_single: cond init");
#endif
  return rq;
}

unsigned int get_queue_pos(const queue* rq, unsigned int no){
  return no * get_size_rule_instance(rq->size_queue_elem);
}


unsigned int get_queue_size(const queue* rq){
  return rq->end - rq->first;
}


/**
   Gets a rule instance at a position in the queue
**/
char* get_rule_instance_single(queue* rq, unsigned int pos){
  return rq->queue + get_queue_pos(rq, pos);
}

const char* get_const_rule_instance_single(const queue* rq, unsigned int pos){
  return rq->queue + get_queue_pos(rq, pos);
}



/**
   Frees memory allocated for the rule queue
**/
void destroy_queue(queue* rq){
#ifdef HAVE_PTHREAD
  pt_err( pthread_mutex_destroy(&rq->queue_mutex), "queue.c: destroy_queue: mutex destroy");
  pt_err( pthread_cond_destroy(&rq->queue_cond),  "queue.c: destroy_queue: cond destroy");
  
#endif
  free(rq);
}

/**
   Assigns position pos in the rule queue the according values.
**/
void assign_rule_queue_instance(queue* rq, unsigned int pos, const axiom* rule, const substitution* sub, unsigned int step){
  assert(pos < rq->end);
  rule_instance* ri = get_rule_instance_single(rq, pos);
  ri->rule = rule;
  copy_substitution_struct(& ri->sub, sub, rq->ssi);
  ri->timestamp = step;
}

/**
   Tests everything in a queue.
   Only for serious debugging
**/
bool test_queue(queue* rq){
  unsigned int i;
  assert(rq->first <= rq->end);
  for(i = rq->first; i < rq->end; i++){
    rule_instance* ri = get_rule_instance_single(rq, i);
    assert(test_rule_instance(ri));
  }
  return true;
}

/**
   Inserts a copy of the substitution into a new entry in the rule queue.
   The original substitution is not changed
**/
rule_instance* push_rule_instance_single(queue ** rq, const axiom* rule, const substitution* sub, unsigned int step, bool clpl_sorted){
  unsigned int pos;
  assert(test_is_instantiation(rule->rhs->free_vars, sub));
#ifdef HAVE_PTHREAD
  lock_queue_single(*rq);
#endif
  assert(test_queue(*rq));
  check_rq_too_small(rq);
  assert(test_queue(*rq));
  if(clpl_sorted){
    for(pos = (*rq)->end
	  ; pos > (*rq)->first 
	  && compare_sub_timestamps(& (get_rule_instance_single(*rq, pos-1))->sub, sub) > 0
	  ; pos--
	)
      copy_rule_instance_struct(get_rule_instance_single(*rq, pos)
				, get_rule_instance_single(*rq, pos-1)
				, (*rq)->ssi
				);
  } else
    pos = (*rq)->end;
  assert(test_queue(*rq));
  (*rq)->end ++;
  assign_rule_queue_instance(*rq, pos, rule, sub, step);
  assert(test_queue(*rq));
#ifdef HAVE_PTHREAD
  unlock_queue_single(*rq);
#endif
  return get_rule_instance_single(*rq, pos);
}



bool queue_is_empty(queue* rq){
  bool is_empty;
#ifdef HAVE_PTHREAD
  lock_queue_single(rq);
#endif
  is_empty = rq->first == rq->end;
#ifdef HAVE_PTHREAD
  unlock_queue_single(rq);
#endif
  return is_empty;
}

/**
   Returns a pointer to the position of a rule_instance

   Note that this pointer may be invalidated on the next call to insert_.. or pop_...
   and the data maybe destroyed on the first backtracking
**/
rule_instance* peek_queue(queue* rq){
  assert(rq->first < rq->end);
  rule_instance* ri = get_rule_instance_single(rq, rq->first);
  assert(test_rule_instance(ri));
  return ri;
}

/**
   Returns timestamp of oldest rule instance
**/
unsigned int queue_age(queue* rq){
  return (peek_queue(rq))->timestamp;
}



/**
   Returns a pointer to the position of a rule_instance

   Note that this pointer may be invalidated on the next call to insert_.. or pop_...
   and the data maybe destroyed on the first backtracking
**/
rule_instance* pop_queue(queue** rqp, unsigned int step){
  queue* rq = *rqp;
#ifdef HAVE_PTHREAD
  lock_queue_single(*rqp);
#endif
  rule_instance* ri = peek_queue(rq);
  assert(test_rule_instance(ri));
  assert(rq->end > rq->first);
  rq->first++;
  rq->n_appl++;
  rq->previous_appl = step;
#ifdef HAVE_PTHREAD
  unlock_queue_single(*rqp);
#endif
  return ri;
}



queue_backup backup_queue(queue* rq){
  queue_backup backup;
  backup.first = rq->first;
  backup.end = rq->end;
  backup.previous_appl = rq->previous_appl;
  backup.n_appl = rq->n_appl;
  return backup;
}

queue* restore_queue(queue* rq, queue_backup* backup){
  rq->first = backup->first;
  rq->end = backup->end;
  rq->previous_appl = backup->previous_appl;
  rq->n_appl = backup->n_appl;
  check_rq_too_large(&rq);
  return rq;
}



unsigned int queue_previous_application(queue* rqs){
  return rqs->previous_appl;
}

/**
   Printing
**/
void print_queue(queue* rq, FILE* f){
  unsigned int i, j;
#ifdef HAVE_PTHREAD
  lock_queue_single(rq);
#endif
  fprintf(f, "queue with %zi entries: \n", get_queue_size(rq));
  for(j=0, i = rq->first; i < rq->end; i++, j++){
    fprintf(f, "\t%i: ", j);
    print_rule_instance(get_const_rule_instance_single(rq, i), f);
    fprintf(f, "\n");
  }
#ifdef HAVE_PTHREAD
  unlock_queue_single(rq);
#endif
}
    
