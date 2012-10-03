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
#include "logger.h"
#include <sys/time.h>
#include <errno.h>

#ifdef HAVE_PTHREAD
/**
   pthread functionality for locking, signaling etc. 
**/
void lock_queue_single(rule_queue_single* rq, const char* filename, int line){
  pt_err(pthread_mutex_lock(& rq->queue_mutex), filename, line ,"rule_queue_single.c: lock_queue_single: mutex_lock");
}

void unlock_queue_single(rule_queue_single* rq, const char* filename, int line){
  pt_err(pthread_mutex_unlock(& rq->queue_mutex),  filename, line, "rule_queue_single.c: unlock_queue_single: mutex_unlock");
}

void wait_queue_single(rule_queue_single* rq, const char* filename, int line){
  pt_err(pthread_cond_wait(& rq->queue_cond, & rq->queue_mutex), filename, line, "rule_queue_single.c: wait_queue_single: cond_wait");
}

bool timedwait_queue_single(rule_queue_single* rq, unsigned int secs, const char* filename, int line){
  struct timeval now;
  struct timespec endwait;
  int retval;
  gettimeofday(&now, NULL);
  endwait.tv_sec = now.tv_sec + secs;
  endwait.tv_nsec = now.tv_usec * 1000;
  retval = pthread_cond_timedwait(& rq->queue_cond, & rq->queue_mutex, &endwait);
  if(retval == ETIMEDOUT)
    return false;
  pt_err(retval, filename, line,  "rule_queue_single.c: timedwait_queue_single: cond_timedwait");
  return true;
}

void signal_queue_single(rule_queue_single* rq, const char* filename, int line){
  pt_err(pthread_cond_signal(& rq->queue_cond),  filename, line, "rule_queue_single.c: signal_queue_single: ");
}


void broadcast_queue_single(rule_queue_single* rq, const char* filename, int line){
  pt_err(pthread_cond_broadcast(& rq->queue_cond), filename, line,   "rule_queue_single.c: broadcast_queue_single:");
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
rule_queue_single* initialize_queue_single(substitution_size_info ssi, unsigned int axiom_no, bool permanent, bool multi_rule_queue){
  rule_queue_single* rq = malloc_tester(sizeof(rule_queue_single));
  rq->size_queue = RULE_QUEUE_INIT_SIZE;
  rq->queue = malloc_tester(get_rq_size_t(rq->size_queue, ssi));
#ifdef HAVE_PTHREAD
  pthread_mutexattr_t mutex_attr;
#endif
  rq->ssi = ssi;
  rq->multi_rule_queue = multi_rule_queue;
  rq->first = 0;
  rq->end = 0;
  rq->previous_appl = 0;
  rq->n_appl = 0;
  rq->axiom_no = axiom_no;
  rq->permanent = permanent;
#ifdef HAVE_PTHREAD
  pt_err(pthread_mutexattr_init(&mutex_attr), __FILE__, __LINE__,  " initialize_queue_single: mutex attr init");
#ifndef NDEBUG
  pt_err(pthread_mutexattr_settype(&mutex_attr, PTHREAD_MUTEX_ERRORCHECK_NP), __FILE__, __LINE__,  " initialize_queue_single: mutex attr settype");
#endif
  pt_err(pthread_mutex_init(& rq->queue_mutex, & mutex_attr),  __FILE__, __LINE__,   " initialize_queue_single: mutex init");
  pt_err(pthread_mutexattr_destroy(&mutex_attr),  __FILE__, __LINE__,   " initialize_queue_single: mutexattr destroy");
  pt_err(pthread_cond_init(& rq->queue_cond, NULL),  __FILE__, __LINE__,   " initialize_queue_single: cond init");
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
  pt_err( pthread_mutex_destroy(&rq->queue_mutex), __FILE__, __LINE__,  "destroy_rule_queue_single: mutex destroy");
  pt_err( pthread_cond_destroy(&rq->queue_cond), __FILE__, __LINE__,   "destroy_rule_queue_single: cond destroy");  
#endif
  free(rq->queue);
  free(rq);
}

/**
   Assigns position pos in the rule queue the according values.
**/
void assign_rule_queue_instance(rule_queue_single* rq, unsigned int pos, const clp_axiom* rule, const substitution* sub, unsigned int step, timestamp_store* ts_store, const constants* cs){
  assert(pos < rq->end);
  rule_instance* ri = get_rule_instance_single(rq, pos);
  ri->rule = rule;
  copy_substitution_struct(& ri->sub, sub, rq->ssi, ts_store, rq->permanent, cs);
  ri->sub_values_ptr = get_sub_values_ptr(&ri->sub);
  ri->timestamp = step;
  ri->used_in_proof = false;
}

/**
   Tests everything in a queue.
   Only for serious debugging

   The queue _must_ be locked before testing.

   Assumes that rule is NULL if the queue takes several axioms
**/
bool test_rule_queue_single(rule_queue_single* rq, const constants* cs){
  unsigned int i;
  assert(rq->first <= rq->end);
  for(i = rq->first; i < rq->end; i++){
#ifndef NDEBUG
    rule_instance* ri = get_rule_instance_single(rq, i);
#endif
    if(!rq->multi_rule_queue){
      assert(ri->rule->axiom_no == rq->axiom_no);
      assert(test_rule_instance(ri, cs));
    }
  }
  return true;
}

/**
   Inserts a copy of the substitution into a new entry in the rule queue.
   The original substitution is not changed
**/
rule_instance* push_rule_instance_single(rule_queue_single * rq, const clp_axiom* rule, const substitution* sub, unsigned int step, bool clpl_sorted, timestamp_store* ts_store, const constants* cs){
  unsigned int pos;
  assert(test_substitution(sub, cs));
#ifdef DEBUG_RETE_INSERT
  FILE* out = get_logger_out(__FILE__, __LINE__);
  fprintf(out, "\nPushing ");
  print_substitution(sub, cs, out);
  fprintf(out, "\n");
  finished_logging(__FILE__, __LINE__);
#endif
#ifdef HAVE_PTHREAD
  lock_queue_single(rq, __FILE__, __LINE__);
#endif
  assert(test_rule_queue_single(rq, cs));
  check_rq_too_small(rq);
  assert(test_rule_queue_single(rq, cs));
  if(clpl_sorted){
    for(pos = rq->end
	  ; pos > rq->first 
	  && compare_sub_timestamps(& (get_rule_instance_single(rq, pos-1))->sub, sub) > 0
	  ; pos--
	)
      copy_rule_instance_struct(get_rule_instance_single(rq, pos)
				, get_rule_instance_single(rq, pos-1)
				, rq->ssi
				, ts_store
				, rq->permanent
				, cs
				);
  } else
    pos = rq->end;
  assert(test_rule_queue_single(rq, cs));
  __sync_add_and_fetch(& rq->end, 1);
  assign_rule_queue_instance(rq, pos, rule, sub, step, ts_store, cs);
  assert(test_rule_queue_single(rq, cs));
#ifdef HAVE_PTHREAD
  signal_queue_single(rq, __FILE__, __LINE__);
  unlock_queue_single(rq, __FILE__, __LINE__);
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
rule_instance* peek_rule_queue_single(rule_queue_single* rq, const constants* cs){
  assert(rq->first < rq->end);
  rule_instance* ri = get_rule_instance_single(rq, rq->first);
  assert(test_rule_instance(ri, cs));
  return ri;
}

/**
   Returns timestamp of oldest rule instance
**/
unsigned int rule_queue_single_age(rule_queue_single* rq, const constants* cs){
  return (peek_rule_queue_single(rq, cs))->timestamp;
}



/**
   Returns a pointer to the position of a rule_instance

   Note that this pointer may be invalidated on the next call to insert_.. or pop_...
   and the data maybe destroyed on the first backtracking
**/
rule_instance* pop_rule_queue_single(rule_queue_single* rq, unsigned int step, const constants* cs){
  rule_instance* ri = peek_rule_queue_single(rq, cs);
  assert(test_rule_instance(ri, cs));
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
void print_rule_queue_single(rule_queue_single* rq, const constants* cs, FILE* f){
  unsigned int i, j;
#ifdef HAVE_PTHREAD
  lock_queue_single(rq, __FILE__, __LINE__);
#endif
  fprintf(f, "queue with %u entries: \n", get_rule_queue_single_size(rq));
  for(j=0, i = rq->first; i < rq->end; i++, j++){
    fprintf(f, "\t%i: ", j);
    print_rule_instance(get_const_rule_instance_single(rq, i), cs, f);
    fprintf(f, "\n");
  }
#ifdef HAVE_PTHREAD
  unlock_queue_single(rq, __FILE__, __LINE__);
#endif
}
    
