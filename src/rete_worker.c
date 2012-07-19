/* rete_worker.c

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
#include "rete_worker.h"
#include "rete_insert_single.h"
#include <errno.h>
#include <string.h>
#include <sys/resource.h>
#include "error_handling.h"
#ifdef HAVE_PTHREAD
#include <pthread.h>


bool worker_pause(rete_worker* worker){
  return worker->pause_signalled && !worker->stop_signalled;
}

bool worker_wait_for_pop(rete_worker*  worker){
  return rete_worker_queue_is_empty(worker->work) && !worker->pause_signalled && !worker->stop_signalled && !worker->recheck_net;
}

bool worker_should_pop(rete_worker* worker){
  return !rete_worker_queue_is_empty(worker->work) && !worker->pause_signalled && !worker->stop_signalled && !worker->recheck_net;
}

bool worker_may_have_new_instance(rete_worker* worker){
  bool retval;
  lock_worker_queue(worker->work, __FILE__, __LINE__);
  retval =  ! rule_queue_single_is_empty(worker->output)
    || ! rete_worker_queue_is_empty(worker->work)
    || rete_worker_is_working(worker);
  unlock_worker_queue(worker->work, __FILE__, __LINE__);
  return retval;
}

/**
   Called by the worker to see if it must reevaluate the beta nodes
**/
bool read_recheck_net(rete_worker* w){
  bool retval = w->recheck_net;
  w->recheck_net = false;
  return retval;
}
/**
   Called by the prover when a new equality is inserted
**/
void set_recheck_net(rete_worker* w){
#ifdef HAVE_PTHREAD
  lock_worker_queue(w->work, __FILE__, __LINE__);
#endif
  w->recheck_net = true;
#ifdef HAVE_PTHREAD
  signal_worker_queue(w->work, __FILE__, __LINE__);
  unlock_worker_queue(w->work, __FILE__, __LINE__);
#endif
}

/**
   Called from rete_worker.
   Waits for new elements to be added to the queue
**/
void worker_thread_pop_worker_queue(rete_worker* worker, const clp_atom** fact, const rete_node ** alpha, unsigned int * step){
  lock_worker_queue(worker->work, __FILE__, __LINE__);
  while(worker_wait_for_pop(worker))
    wait_worker_queue(worker->work, __FILE__, __LINE__);
  if(worker_should_pop(worker)){
    pop_rete_worker_queue(worker->work, fact, alpha, step);
    __sync_lock_test_and_set(&worker->state, has_popped);
    worker->step = *step;
  }
  unlock_worker_queue(worker->work, __FILE__, __LINE__);
}

/**
   This is called when facts are not inserted because the test in the alpha nodes fails.
   These facts are put in a queue and then reinserted whenever the beta node needs rechecking

   This is because new equalities may make them pass the alpha tests

   This queue does not require locking, since it is only accessed by the one worker using it
**/
void insert_worker_uninserted_queue(rete_worker* worker, const clp_atom * fact, const rete_node * alpha, unsigned int step){
  push_rete_worker_queue(worker->uninserted, fact, alpha, step);
}

/**
   Reinserts all elements from the uninserted queue. Called when rechecking net
**/
void worker_reinsert_uninserted(rete_worker* worker){
  unsigned int start_size;
  const clp_atom* fact;
  const rete_node* node;
  unsigned int step;
  substitution* tmp_sub = create_empty_substitution(worker->net->th, worker->tmp_subs);
  for(start_size = get_rete_worker_queue_size(worker->uninserted); start_size > 0; start_size--){
    pop_rete_worker_queue(worker->uninserted, &fact, &node, &step);
    worker->step = step;
    init_substitution(tmp_sub, worker->net->th, step, worker->timestamp_store);
    if (!insert_rete_alpha_fact_single(worker->net, worker->node_subs, worker->tmp_subs, worker->timestamp_store, worker->output, node, fact, step, tmp_sub, *(worker->constants)) )
      insert_worker_uninserted_queue(worker, fact, node, step);
  }
}

/**
   The main routine of the queue worker

   This is set to asynchronous canceling, since the data it changes is 
   not used after cancelation. 

   The main loop runs for the whole runtime of the prover (stop_signalled arrives
   at the very end.)
   The first internal loop runs when the worker is paused, during treatment of disjunctions.

   The signalling in the end of the loop is either to signal the prover that a new rule instance is arriving, 
   or that it is again waiting, such that the backup process in the disjunction treatment can continue.
**/
void * queue_worker_routine(void* arg){
  const rete_node* alpha;
  const clp_atom* fact;
  unsigned int step;
  rete_worker * worker = arg;
  substitution* tmp_sub = create_empty_substitution(worker->net->th, worker->tmp_subs);
  while(!worker->stop_signalled){
    lock_worker_queue(worker->work, __FILE__, __LINE__);
    while(worker_pause(worker))
      wait_worker_queue(worker->work, __FILE__, __LINE__);
    unlock_worker_queue(worker->work, __FILE__, __LINE__);
    if(worker->stop_signalled)
      break;
    if(read_recheck_net(worker)){
      worker->state = has_popped;
#ifdef __DEBUG_RETE_STATE
      printf("Rechecking relevant parts of rete net for axiom %s.\n", worker->net->th->axioms[worker->axiom_no]->name);
#endif
      recheck_beta_node(worker->net, worker->node_subs, worker->tmp_subs, worker->timestamp_store, worker->net->rule_nodes[worker->axiom_no], worker->output, step, *(worker->constants));
      worker_reinsert_uninserted(worker);
    } else {
      worker_thread_pop_worker_queue(worker, &fact, &alpha, & step);
      if(worker->state == has_popped){
	init_substitution(tmp_sub, worker->net->th, step, worker->timestamp_store);
	if (!insert_rete_alpha_fact_single(worker->net, worker->node_subs, worker->tmp_subs, worker->timestamp_store, worker->output, alpha, fact, step, tmp_sub, *(worker->constants)) )
	  insert_worker_uninserted_queue(worker, fact, alpha, step);
      }
    }
    __sync_lock_test_and_set(& worker->state, waiting);
    if(!worker->pause_signalled && !worker->stop_signalled){
      lock_queue_single(worker->output, __FILE__, __LINE__);
      signal_queue_single(worker->output, __FILE__, __LINE__);
      unlock_queue_single(worker->output, __FILE__, __LINE__);
    } else {
      lock_worker_queue(worker->work, __FILE__, __LINE__);
      signal_worker_queue(worker->work, __FILE__, __LINE__);
      unlock_worker_queue(worker->work, __FILE__, __LINE__);
    }
  }
  return NULL;
}

/**
   The functions for starting the thread
   Separated not to make init_rete_worker messy
**/
void start_worker_thread(rete_worker* worker){
  pthread_attr_t attr;
  int errval;
  size_t stacksize = 10000;
  pt_err(pthread_attr_init(&attr), __FILE__, __LINE__, "rete_worker.c: start_worker_thread: attr init");
  pthread_attr_setstacksize(&attr, stacksize); 
  errval = pthread_create(& worker->tid, &attr, queue_worker_routine, worker);
  if(errval != 0){
    fprintf(stderr,"%s: line %i: start_worker_thread: %s\n", __FILE__, __LINE__, strerror(errval));
    if(errval == EAGAIN){
      fprintf(stderr, "Insufficient resources to run all the threads in the rete network. Try increasing the limits on number of processes or on the virtual memory. You can also try limiting the stack memory, by running ulimit -Ss <limit>\n");
      show_limit(RLIMIT_NPROC, stdout, "Process");
      show_limit(RLIMIT_STACK, stdout, "Stack");
    }
  }
  pt_err(errval, __FILE__, __LINE__, "rete_worker.c: start_worker_thread: create thread");
  pt_err(pthread_attr_destroy(&attr), __FILE__, __LINE__, "rete_worker.c: start_worker_thread: attr init");
}
/**
   Creates the initial queue containing
   all facts from the theory

   Also allocates memory for the qeue
 **/
rete_worker* init_rete_worker(const rete_net* net, unsigned int axiom_no, substitution_store_mt * tmp_subs, substitution_store_array * node_subs, timestamp_store* timestamp_store, rule_queue_single * output, rete_worker_queue * work, constants** cs){
  rete_worker * worker = (rete_worker *) malloc_tester(sizeof(rete_worker));
  worker->work = work;
  worker->output = output;
  worker->state = waiting;
  worker->stop_signalled = false;
  worker->pause_signalled = false;
  worker->net = net;
  worker->tmp_subs = tmp_subs;
  worker->node_subs = node_subs;
  worker->timestamp_store = timestamp_store;
  worker->axiom_no = axiom_no;
  worker->constants = cs;
  worker->recheck_net = false;
  worker->uninserted = init_rete_worker_queue();
  start_worker_thread(worker);
  return worker;
}

/**
   This is the only function that writes to the
   componend stop_signalled
   Called from rete_state_single when destroying the state
**/
void stop_rete_worker(rete_worker* worker){
  void* t_retval;
  int join_rv;
  if(!worker->stop_signalled){
    worker->stop_signalled = true;
    lock_worker_queue(worker->work, __FILE__, __LINE__);
    signal_worker_queue(worker->work, __FILE__, __LINE__);
    unlock_worker_queue(worker->work, __FILE__, __LINE__);
    
    join_rv = pthread_join(worker->tid, &t_retval);
    if(join_rv != 0)
      perror("rete_worker.c: stop_rete_worker: Error when joining with worker thread:"); 
  }
}

/**
   Called before backing up the queues, 
   and before restoring a backup
**/
void pause_rete_worker(rete_worker* worker){
  bool already_paused = __sync_lock_test_and_set(&worker->pause_signalled, true);
  assert(!already_paused);
  lock_worker_queue(worker->work, __FILE__, __LINE__);
  signal_worker_queue(worker->work, __FILE__, __LINE__);
  unlock_worker_queue(worker->work, __FILE__, __LINE__);
}

/**
   Called after restoring a backup
**/
void restart_rete_worker(rete_worker* rq){
  rq->stop_signalled = false;
  start_worker_thread(rq);
}

/**
   Called after backing up the queues
**/
void continue_rete_worker(rete_worker* worker){
  bool was_paused = __sync_lock_test_and_set(&worker->pause_signalled, false);
  assert(was_paused);
  lock_worker_queue(worker->work, __FILE__, __LINE__);
  signal_worker_queue(worker->work, __FILE__, __LINE__);
  unlock_worker_queue(worker->work, __FILE__, __LINE__);
}


/**
   Frees memory allocated for the rule queue
**/
void destroy_rete_worker(rete_worker* rq){
  stop_rete_worker(rq);
  free(rq);
}

bool rete_worker_is_working(rete_worker* rq){
  return rq->state != waiting;
}

/**
   Should be called after pause_worker, waits until the 
   worker has returned from the matching/insertion and 
   is again paused
**/
void wait_for_worker_to_pause(rete_worker* worker){
  lock_worker_queue(worker->work, __FILE__, __LINE__);
  while(worker->state != waiting)
    wait_worker_queue(worker->work, __FILE__, __LINE__);
  unlock_worker_queue(worker->work, __FILE__, __LINE__);
}

unsigned int get_worker_step(rete_worker* rq){
  return rq->step;
}

#endif
