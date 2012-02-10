/* rule_instance.h

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
#ifndef __INCLUDED_RULE_INSTANCE_H
#define __INCLUDED_RULE_INSTANCE_H

#include "substitution_struct.h"
#include "substitution_size_info.h"
#include "axiom.h"

/**
   branch is only used by the instances in the "history"
**/
typedef struct rule_instance_t {
  unsigned int timestamp;
  const axiom * rule;
  bool used_in_proof;
  substitution sub;
} rule_instance;

rule_instance* create_rule_instance(const axiom*, const substitution*, substitution_size_info, timestamp_store*);
rule_instance* create_dummy_rule_instance(substitution_size_info);
void delete_dummy_rule_instance(rule_instance*);
void copy_rule_instance_struct(rule_instance* dest, const rule_instance* orig, substitution_size_info ssi, timestamp_store*);
rule_instance* copy_rule_instance(const rule_instance* orig, substitution_size_info ssi, timestamp_store*);

bool test_rule_instance(const rule_instance*);

void print_rule_instance(const rule_instance*, const constants*, FILE*);
void print_dot_rule_instance(const rule_instance*, const constants*, FILE*);
void print_coq_rule_instance(const rule_instance*, const constants*, FILE*);

#endif
