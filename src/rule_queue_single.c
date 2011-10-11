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

size_t get_rq_size_t(size_t size_queue, substitution_size_info ssi){
  return sizeof(rule_queue_single) + size_queue * (sizeof(rule_instance_single) + sizeof(unsigned int) * get_size_timestamps(ssi));
}

void resize_rq_size(rule_queue_single** rq){
  *rq = realloc_tester(*rq, get_rq_size_t((*rq)->size_queue, (*rq)->ssi));
}

void check_rq_too_large(rule_queue_single** rq){
  while((*rq)->end < (*rq)->size_queue / 2 && (*rq)->size_queue > 20){
    (*rq)->size_queue /= 2;
    resize_rq_size(rq);
  }
}

void check_rq_too_small(rule_queue_single** rq){
  if((*rq)->end + 1 >= (*rq)->size_queue){
    (*rq)->size_queue *= 2;
    resize_rq_size(rq);
  }
}




/**
   Creates the initial queue containing
   all facts from the theory
 **/
rule_queue_single* initialize_queue_single(substitution_size_info ssi){
  unsigned int i;
  size_t size_queue = 10;
  rule_queue_single* rq = malloc_tester(get_rq_size_t(size_queue, ssi));
  rq->ssi = ssi;
  rq->first = 0;
  rq->end = 0;
  rq->previous_appl = 0;
  rq->n_appl = 0;
  rq->size_queue = size_queue;

  return rq;
}

/**
   Inserts a copy of the substitution into a new entry in the rule queue.
   The original substitution is not changed
**/
rule_queue_single* insert_rule_instance_single(rule_queue_single* rq, const axiom* rule, const substitution* sub, unsigned int step){
  check_rq_too_small(&rq);
  rq->queue[rq->end].rule = rule;
  rq->queue[rq->end].sub = * sub;
  rq->queue[rq->end].timestamp = step;
  memcpy(rq->queue[rq->end].sub_ts, sub->timestamps, get_size_timestamps(rq->ssi) * sizeof(unsigned int));
  rq->queue[rq->end].sub.timestamps = rq->queue[rq->end].sub_ts;
  rq->end ++;
  return rq;
}

bool rule_queue_single_empty(rule_queue_single* rq){
  return rq->first == rq->end;
}

/**
   Returns a pointer to the position of a rule_instance_single

   Note that this pointer may be invalidated on the next call to insert_.. or pop_...
   and the data maybe destroyed on the first backtracking
**/
const rule_instance_single* pop_rule_queue_single(rule_queue_single** rqp){
  rule_queue_single* rq = *rqp;
  assert(rq->end > rq->first);
  rq->end--;
  check_rq_too_large(rqp);
  return & ((*rqp)->queue[(*rqp)->end]);
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
  check_rq_too_large(&rq);
  return rq;
}


/**
   Called from strategy.c
**/
substitution* get_rule_instance_single_subsitution(rule_instance_union ri){
  return & (ri.single->sub);
}
