/* rule_queue_single.h

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
#ifndef __INCLUDED_RULE_QUEUE_SINGLE_H
#define __INCLUDED_RULE_QUEUE_SINGLE_H

#include "rule_instance_single.h"
#include "substitution_size_info.h"

/**
   The queue of rule instances to be treated
   Called "conflict set" by Forgy

   Is a fifo, implemented by an array

   first is the index of the oldest element, while end is the next free index

   previous_appl is used by the strategy, and indicates the
   timestamp of the previous time instances were removed from the queue. 
   n_appl indicates how many times instances have been removed from the queue.
**/
typedef struct rule_queue_single_t {
  size_t n_queue;
  size_t size_queue;
  size_t first;
  size_t end;
  unsigned int previous_appl;
  unsigned int n_appl;
  rule_instance_single queue[];
} rule_queue_single;


typedef struct rule_queue_single_backup_t {
  unsigned int first;
  unsigned int end;
  unsigned int previous_appl;
  unsigned int n_appl;
} rule_queue_single_backup;

rule_queue_single* initialize_queue_single(substitution_size_info);
void insert_rule_instance_single(rule_queue_single*, axiom*, substitution*);
rule_instance_single* pop_rule_queue_single(rule_queue_single*);
rule_queue_single_backup backup_rule_queue_single(rule_queue_single*);
void restore_rule_queue_single(rule_queue_single*, rule_queue_single_backup*);

#endif
