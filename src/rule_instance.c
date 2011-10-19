/* rule_instance.c

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

#include "rule_instance.h"


void copy_rule_instance_struct(rule_instance* dest, const rule_instance* orig, substitution_size_info ssi){
  copy_substitution_struct(& dest->sub, & orig->sub, ssi);
  dest->rule = orig->rule;
  dest->timestamp = orig->timestamp;
  dest->used_in_proof = orig->used_in_proof;
}



/*
  When the prover changes "used_in_proof" to true, it must copy the rule instance, 
  as the other copies must not be changed
*/
rule_instance* copy_rule_instance(const rule_instance* orig, substitution_size_info ssi){
  rule_instance* copy = malloc_tester(get_size_rule_instance(ssi));
  copy_rule_instance_struct(copy, orig, ssi);
  return copy;
}



/**
   External function for creating a rule instance
   
   Only used when commandline option fact-set is on, 
   that is, when RETE is not used
**/
rule_instance* create_rule_instance(const axiom* rule, const substitution* sub, substitution_size_info ssi){
  rule_instance* ins = malloc_tester(get_size_rule_instance(ssi));

  assert(test_substitution(sub));
  
  // The timestamp on rule instances is only used by RETE. Otherwise, rule instances are immediately added to the factset
  ins->timestamp = 0;
  ins->rule = rule;

  copy_substitution_struct(&(ins->sub), sub, ssi);
  ins->used_in_proof = false;
  return ins;
}



/**
   Used by prover as a stack marker in disj_ri_stack
**/
rule_instance* create_dummy_rule_instance(substitution_size_info ssi){
  rule_instance* ins = malloc_tester(get_size_rule_instance(ssi));
  ins->timestamp = 0;
  ins->rule = NULL;
  ins->used_in_proof = false;
  return ins;
}
  
void delete_dummy_rule_instance(rule_instance* ri){
  free(ri);
}

// Basic sanity testing

bool test_rule_instance(const rule_instance* ri){
  assert(ri != NULL);
  assert(ri->rule != NULL);
  assert(test_substitution(& ri->sub));
  assert(test_axiom(ri->rule, ri->rule->axiom_no));
  if(!test_is_instantiation(ri->rule->rhs->free_vars, & (ri->sub))){
    fprintf(stderr, "An incorrect rule instance added to the queue\n");
    print_rule_instance(ri, stderr);
    return false;
  }
  return true;
}

// Output functions
   
void print_coq_rule_instance(const rule_instance *ri, FILE* f){
  fprintf(f, "%s", ri->rule->name); 
  print_coq_substitution(& ri->sub, ri->rule->lhs->free_vars, f);
}


void print_dot_rule_instance(const rule_instance *ri, FILE* f){
  fprintf(f, " %s (", ri->rule->name); 
  print_dot_axiom(ri->rule, f);
  fprintf(f, ") ");
  print_substitution(& ri->sub, f);
}

void print_rule_instance(const rule_instance *ri, FILE* f){
  fprintf(f, " %s (", ri->rule->name); 
  print_fol_axiom(ri->rule, f);
  fprintf(f, ") ");
  print_substitution(& ri->sub, f);
  if(ri->rule->is_existential){
    fprintf(f, " - existential: ");
    
  } else
    fprintf(f, " - definite");
}