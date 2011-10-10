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


insert_rule_instance(rule_queue_single*, axiom*, substitution*);
rule_instance_single* pop_rule_queue_single(rule_queue_single*);
rule_queue_single_backup backup_rule_queue_single(rule_queue_single*);
void restore_rule_queue_single(rule_queue_single*, rule_queue_single_backup*);


/**
   Creates the initial queue containing
   all facts from the theory
 **/
rule_queue_single* initialize_queue_single(substitution_size_info ssi){
  unsigned int i;
  size_t size_queue = 10;
  rule_queue_single* rq = malloc_tester(sizeof(rule_queue_single) + size_queue * (sizeof(rule_instance_single) + sizeof(unsigned int) * get_size_timestamps(ssi)));

  rq->n_queue = 0;
  rq->first = 0;
  rq->end = 0;
  rq->previous_appl = 0;
  rq->n_appl = 0;
  rq->size_queue = size_queue;

  return rq;
}

insert_rule_instance_single(rule_queue_single*, axiom*, substitution*){
rule_instance_single* pop_rule_queue_single(rule_queue_single*);
rule_queue_single_backup backup_rule_queue_single(rule_queue_single*);
void restore_rule_queue_single(rule_queue_single*, rule_queue_single_backup*);
