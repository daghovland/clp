/* rule_queue_single.c

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
#include "rule_queue_single.h"
#include "rule_queue_state.h"
#include "rule_instance.h"
#include "substitution.h"
#include "error_handling.h"


#ifdef HAVE_PTHREAD
/**
   pthread functionality for locking, signaling etc. 
**/
void lock_queue_single(rule_queue_single* rq){
  pt_err(pthread_mutex_lock(& rq->queue_mutex), "rule_queue_single.c: lock_queue_single: mutex_lock");
}

void unlock_queue_single(rule_queue_single* rq){
  pt_err(pthread_mutex_unlock(& rq->queue_mutex), "rule_queue_single.c: unlock_queue_single: mutex_unlock");
}

void wait_queue_single(rule_queue_single* rq){
  pt_err(pthread_cond_wait(& rq->queue_cond, & rq->queue_mutex), "rule_queue_single.c: wait_queue_single: cond_wait");
}

void signal_queue_single(rule_queue_single* rq){
  pt_err(pthread_cond_signal(& rq->queue_cond), "rule_queue_single.c: signal_queue_single: ");
}


void broadcast_queue_single(rule_queue_single* rq){
  pt_err(pthread_cond_broadcast(& rq->queue_cond), "rule_queue_single.c: broadcast_queue_single:");
}
#endif

size_t get_rq_size_t(size_t size_queue, substitution_size_info ssi){
  return size_queue * get_size_rule_instance(ssi);
}

void resize_rq_size(rule_queue_single* rq){
  rq->queue = realloc_tester(rq->queue, get_rq_size_t(rq->size_queue, rq->ssi));
}

void check_rq_too_large(rule_queue_single* rq){
  while(rq->end < rq->size_queue / 2 && rq->size_queue > 20){
    rq->size_queue /= 2;
    resize_rq_size(rq);
  }
}

void check_rq_too_small(rule_queue_single* rq){
  if(rq->end + 1 >= rq->size_queue){
    rq->size_queue *= 2;
    resize_rq_size(rq);
  }
}

/**
   Creates the initial queue containing
   all facts from the theory

   Also allocates memory for the qeue

   The axiom_no is only for debugging purposes
 **/
rule_queue_single* initialize_queue_single(substitution_size_info ssi, unsigned int axiom_no){
  rule_queue_single* rq = malloc_tester(sizeof(rule_queue_single));
  rq->size_queue = RULE_QUEUE_INIT_SIZE;
  rq->queue = malloc_tester(get_rq_size_t(rq->size_queue, ssi));
#ifdef HAVE_PTHREAD
  pthread_mutexattr_t mutex_attr;
#endif
  rq->ssi = ssi;
  rq->first = 0;
  rq->end = 0;
  rq->previous_appl = 0;
  rq->n_appl = 0;
  rq->axiom_no = axiom_no;
#ifdef HAVE_PTHREAD
  pt_err(pthread_mutexattr_init(&mutex_attr), "rule_queue_single.c: initialize_queue_single: mutex attr init");
#ifndef NDEBUG
  pt_err(pthread_mutexattr_settype(&mutex_attr, PTHREAD_MUTEX_ERRORCHECK_NP),  "rule_queue_single.c: initialize_queue_single: mutex attr settype");
#endif
  pt_err(pthread_mutex_init(& rq->queue_mutex, & mutex_attr),   "rule_queue_single.c: initialize_queue_single: mutex init");
  pt_err(pthread_mutexattr_destroy(&mutex_attr),   "rule_queue_single.c: initialize_queue_single: mutexattr destroy");
  pt_err(pthread_cond_init(& rq->queue_cond, NULL),   "rule_queue_single.c: initialize_queue_single: cond init");
#endif
  return rq;
}

unsigned int get_rule_queue_single_pos(const rule_queue_single* rq, unsigned int no){
  return no * get_size_rule_instance(rq->ssi);
}


unsigned int get_rule_queue_single_size(const rule_queue_single* rq){
  return rq->end - rq->first;
}


/**
   Gets a rule instance at a position in the queue
**/
rule_instance* get_rule_instance_single(rule_queue_single* rq, unsigned int pos){
  return (rule_instance*) (rq->queue + get_rule_queue_single_pos(rq, pos));
}

const rule_instance* get_const_rule_instance_single(const rule_queue_single* rq, unsigned int pos){
  return (const rule_instance*) (rq->queue + get_rule_queue_single_pos(rq, pos));
}



/**
   Frees memory allocated for the rule queue
**/
void destroy_rule_queue_single(rule_queue_single* rq){
#ifdef HAVE_PTHREAD
  pt_err( pthread_mutex_destroy(&rq->queue_mutex), "rule_queue_single.c: destroy_rule_queue_single: mutex destroy");
  pt_err( pthread_cond_destroy(&rq->queue_cond),  "rule_queue_single.c: destroy_rule_queue_single: cond destroy");
  
#endif
  free(rq);
}

/**
   Assigns position pos in the rule queue the according values.
**/
void assign_rule_queue_instance(rule_queue_single* rq, unsigned int pos, const axiom* rule, const substitution* sub, unsigned int step){
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
bool test_rule_queue_single(rule_queue_single* rq){
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
rule_instance* push_rule_instance_single(rule_queue_single * rq, const axiom* rule, const substitution* sub, unsigned int step, bool clpl_sorted){
  unsigned int pos;
  assert(test_is_instantiation(rule->rhs->free_vars, sub));
#ifdef HAVE_PTHREAD
  lock_queue_single(rq);
#endif
  assert(test_rule_queue_single(rq));
  check_rq_too_small(rq);
  assert(test_rule_queue_single(rq));
  if(clpl_sorted){
    for(pos = rq->end
	  ; pos > rq->first 
	  && compare_sub_timestamps(& (get_rule_instance_single(rq, pos-1))->sub, sub) > 0
	  ; pos--
	)
      copy_rule_instance_struct(get_rule_instance_single(rq, pos)
				, get_rule_instance_single(rq, pos-1)
				, rq->ssi
				);
  } else
    pos = rq->end;
  assert(test_rule_queue_single(rq));
  rq->end ++;
  assign_rule_queue_instance(rq, pos, rule, sub, step);
  assert(test_rule_queue_single(rq));
#ifdef HAVE_PTHREAD
  signal_queue_single(rq);
  unlock_queue_single(rq);
#endif
  return get_rule_instance_single(rq, pos);
}



bool rule_queue_single_is_empty(rule_queue_single* rq){
  return rq->first == rq->end;
}

/**
   Returns a pointer to the position of a rule_instance

   Note that this pointer may be invalidated on the next call to insert_.. or pop_...
   and the data maybe destroyed on the first backtracking
**/
rule_instance* peek_rule_queue_single(rule_queue_single* rq){
  assert(rq->first < rq->end);
  rule_instance* ri = get_rule_instance_single(rq, rq->first);
  assert(test_rule_instance(ri));
  return ri;
}

/**
   Returns timestamp of oldest rule instance
**/
unsigned int rule_queue_single_age(rule_queue_single* rq){
  return (peek_rule_queue_single(rq))->timestamp;
}



/**
   Returns a pointer to the position of a rule_instance

   Note that this pointer may be invalidated on the next call to insert_.. or pop_...
   and the data maybe destroyed on the first backtracking
**/
rule_instance* pop_rule_queue_single(rule_queue_single* rq, unsigned int step){
  rule_instance* ri = peek_rule_queue_single(rq);
  assert(test_rule_instance(ri));
  assert(rq->end > rq->first);
  rq->first++;
  rq->n_appl++;
  rq->previous_appl = step;
  return ri;
}



rule_queue_single_backup backup_rule_queue_single(rule_queue_single* rq){
  rule_queue_single_backup backup;
  backup.first = rq->first;
  backup.end = rq->end;
  backup.previous_appl = rq->previous_appl;
  backup.n_appl = rq->n_appl;
  return backup;
}

rule_queue_single* restore_rule_queue_single(rule_queue_single* rq, rule_queue_single_backup* backup){
  rq->first = backup->first;
  rq->end = backup->end;
  rq->previous_appl = backup->previous_appl;
  rq->n_appl = backup->n_appl;
  check_rq_too_large(rq);
  return rq;
}



unsigned int rule_queue_single_previous_application(rule_queue_single* rqs){
  return rqs->previous_appl;
}

/**
   Printing
**/
void print_rule_queue_single(rule_queue_single* rq, FILE* f){
  unsigned int i, j;
#ifdef HAVE_PTHREAD
  lock_queue_single(rq);
#endif
  fprintf(f, "queue with %zi entries: \n", get_rule_queue_single_size(rq));
  for(j=0, i = rq->first; i < rq->end; i++, j++){
    fprintf(f, "\t%i: ", j);
    print_rule_instance(get_const_rule_instance_single(rq, i), f);
    fprintf(f, "\n");
  }
#ifdef HAVE_PTHREAD
  unlock_queue_single(rq);
#endif
}
    
