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


#ifdef HAVE_PTHREAD


/**
   Called from rete_worker.
   Waits for new elements to be added to the queue
**/
void worker_thread_pop_worker_queue(rete_worker* worker, const atom** fact, const rete_node ** alpha, unsigned int * step){
  lock_worker_queue(worker->work);
  while(rete_worker_queue_is_empty(worker->work) && !worker->stop_worker)
    wait_worker_queue(worker->work);
  if(!worker->stop_worker){
    worker->working = true;
    pop_rete_worker_queue(worker->work, fact, alpha, step);
    worker->step = *step;
  }
  unlock_worker_queue(worker->work);
}



/**
   The main routine of the queue worker

   This is set to asynchornous canceling, since the data it changes is 
   not used after cancelation. 
**/
void * queue_worker_routine(void* arg){
  int oldtype;
  const rete_node* alpha;
  const atom* fact;
  unsigned int step;
  rete_worker * worker = arg;
  substitution* tmp_sub = create_empty_substitution(worker->net->th, worker->tmp_subs);
  //pt_err(pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, &oldtype), "queue.c: queue_worker_routine: set cancel type");
  while(!worker->stop_worker){
    worker_thread_pop_worker_queue(worker, &fact, &alpha, & step);
    if(worker->stop_worker)
      break;
    init_substitution(tmp_sub, worker->net->th, step);
    insert_rete_alpha_fact_single(worker->net, worker->node_subs, worker->tmp_subs, worker->output, alpha, fact, step, tmp_sub);
    if(worker->stop_worker)
      break;
    worker->working = false;
    lock_queue_single(worker->output);
    signal_queue_single(worker->output);
    unlock_queue_single(worker->output);
  }
  return NULL;
}


/**
   The functions for starting the thread
   Separated not to make init_rete_worker messy
**/
void start_worker_thread(rete_worker* worker){
  pthread_attr_t attr;
  pt_err(pthread_attr_init(&attr), "rete_worker.c: start_worker_thread: attr init");
  pt_err(pthread_create(& worker->tid, &attr, queue_worker_routine, worker), "rete_worker.c:init_rete_worker: thread create");
  pt_err(pthread_attr_destroy(&attr), "rete_worker.c: start_worker_thread: attr init");
}
/**
   Creates the initial queue containing
   all facts from the theory

   Also allocates memory for the qeue
 **/
rete_worker* init_rete_worker(const rete_net* net, substitution_store_mt * tmp_subs, substitution_store_array * node_subs, rule_queue_single * output, rete_worker_queue * work){
  rete_worker * worker = (rete_worker *) malloc_tester(sizeof(rete_worker));
  worker->work = work;
  worker->output = output;
  worker->working = false;
  worker->stop_worker = false;
  worker->net = net;
  worker->tmp_subs = tmp_subs;
  worker->node_subs = node_subs;
  start_worker_thread(worker);
  return worker;
}


void stop_rete_worker(rete_worker* worker){
  void* t_retval;
  worker->stop_worker = true;

  lock_worker_queue(worker->work);
  signal_worker_queue(worker->work);
  if(worker->working)
    unpop_rete_worker_queue(worker->work);
  unlock_worker_queue(worker->work);

  pthread_join(worker->tid, &t_retval);
}

/**
   Called before backing up the queues, 
   and before restoring a backup
**/
void pause_rete_worker(rete_worker* worker){
  lock_worker_queue(worker->work);
  lock_queue_single(worker->output);
  if(worker->working)
    unpop_rete_worker_queue(worker->work);
}

/**
   Called after restoring a backup
**/
void restart_rete_worker(rete_worker* rq){
  rq->stop_worker = false;
  rq->working = false;
  start_worker_thread(rq);
}

/**
   Called after backing up the queues
**/
void continue_rete_worker(rete_worker* worker){
  unlock_worker_queue(worker->work);
  unlock_queue_single(worker->output);
}


/**
   Frees memory allocated for the rule queue
**/
void destroy_rete_worker(rete_worker* rq){
  stop_rete_worker(rq);
  free(rq);
}

bool rete_worker_is_working(rete_worker* rq){
  return rq->working;
}

unsigned int get_worker_step(rete_worker* rq){
  return rq->step;
}

#endif
