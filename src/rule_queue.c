/* rule_queue.c

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
#include "rule_queue.h"
#include "proof_writer.h"
/**
   Creates the initial queue containing
   all facts from the theory
 **/
rule_queue* initialize_queue(void){
  unsigned int i;
  size_t size_queue = 10;
  rule_queue* rq = malloc_tester(sizeof(rule_queue) + size_queue * sizeof(rule_instance*));

  rq->n_queue = 0;
  rq->first = 0;
  rq->end = 0;
  rq->previous_appl = 0;
  rq->n_appl = 0;
  rq->size_queue = size_queue;

  for(i = 0; i < rq->size_queue; i++)
    rq->queue[i] = NULL;
  return rq;
}

/**
   Called from split_rete_state in rete_state.c
**/
rule_queue* copy_rule_queue(const rule_queue* rq){
  rule_queue* copy = malloc_tester(sizeof(rule_queue) + (rq->size_queue * sizeof(rule_instance*)));
  memcpy(copy, rq, sizeof(rule_queue) + (rq->size_queue * sizeof(rule_instance*)));
  return copy;
}


/**
   Destructor of copied rule queue. 
   Called when a split is finished, from delete_rete_state in rete_state.c
**/
void delete_rule_queue_before(rule_queue* rq, rule_queue* orig){
  unsigned int i;
  for(i = rq->first; rq->first != orig->first && rq->first != rq->end; i = (i+1) % rq->size_queue){
    free(rq->queue[i]);
    rq->queue[i] = NULL;
  }
  free(rq);
}


/**
   Testing sanity of rule queue. Sort of unit testing. Returns true if queue is sane
**/
bool test_rule_queue(const rule_queue* rq, const rete_net_state* state){
  rule_instance* ri;  
  unsigned int i, timestamp = 0;
  if(rq == NULL || rq->queue == NULL){
    fprintf(stderr, "NULL queue\n");
    return false;
  }
  ri = rq->queue[rq->size_queue-1];
  if (rq->end != (rq->first + rq->n_queue) % rq->size_queue){
    fprintf(stderr, "Error in rule queue size vs. start/end information\n");
    return false;
  }
  for(i = rq->first; i != rq->end; i = (i+1) % rq->size_queue){
    rule_instance* ri = rq->queue[i];
    if(timestamp > ri->timestamp){
      fprintf(stderr, "Wrong timestamp ordering at queue position %i (previous was: %i, this was: %i)\n", i, timestamp, ri->timestamp);
      return false;
    }
    timestamp = ri->timestamp;
    if(!test_rule_instance(rq->queue[i], state)){
      fprintf(stderr, "Invalid rule instance at queue position %i\n", i);
      return false;
    }
  } // end for(i = ...)
  return true;
}

/**
   Internal function for comparing timestamps on a substition

   They correspond to the times at which the matching for each conjunct
   was introduced to the factset

   Returns positive if first is larger(newer) than last, negative if first is smaller(newer) than last,
   and 0 if they are equal
**/
int compare_timestamps(const substitution* first, const substitution* last){
  assert(first->n_timestamps == last->n_timestamps);
  unsigned int i;
  for(i = 0; i < first->n_timestamps; i++){
    if(first->timestamps[i] != last->timestamps[i])
      return first->timestamps[i] - last->timestamps[i];
  }
  return 0;
}

/**
   Internal function for adding a rule instance to a queue

   The queue must be returned, since realloc might change the address
**/
rule_queue* _add_rule_to_queue(rule_instance* ri, rule_queue* rq, bool clpl_sorted){
  size_t orig_size = rq->size_queue;

  rq->n_queue++;
  if(rq->n_queue >= rq->size_queue){    
    rq->size_queue *= 2;
    rq = realloc_tester(rq, sizeof(rule_queue) + (rq->size_queue * sizeof(rule_instance*)));
    if(rq->end < rq->first){
      assert(orig_size == rq->size_queue / 2);
      assert(rq->end < orig_size);
      if(rq->end > 0)
	memcpy(& rq->queue[orig_size],  rq->queue, rq->end * sizeof(rule_instance*));
      rq->end += orig_size;
    }
  }
  if(clpl_sorted){
    int i = rq->end;
    int j = (rq->end + rq->size_queue - 1) % rq->size_queue;
    while(i != rq->first && compare_timestamps(rq->queue[j]->substitution, ri->substitution) > 0){
      rq->queue[i] = rq->queue[j];
      i = j;
      j = (i + rq->size_queue - 1) % rq->size_queue;
    }
    rq->queue[i] = ri;
  } else {
    rq->queue[rq->end] = ri;
  }
  rq->end++;
  rq->end %= rq->size_queue;
  return rq;
}


rule_instance* _pop_rule_queue(rule_queue* rq){
  rule_instance* retval;

  assert(rq->n_queue > 0);
  assert(rq->first != rq->end);

  rq->n_queue--;
  retval =  rq->queue[rq->first++];
  rq->first %= rq->size_queue;

  return retval;
}

/**
   Takes most recently added instance from rule queue

   Used by CL.pl emulation to choose next instance
**/
rule_instance* _pop_youngest_rule_queue(rule_queue* rq){
  rule_instance* retval;

  assert(rq->n_queue > 0);
  assert(rq->first != rq->end);

  rq->n_queue--;
  if(rq->end == 0)
    rq->end = rq->size_queue-1;
  else
    rq->end--;

  retval =  rq->queue[rq->end];

  return retval;
}

/**
   Removes the rule instance ri from queue rq. 
   Called only when you _know_ that ri is in rq, 
   for example after taking it from one of the axiom rule queues

   Does not free ri, the caller function must do this afterwards
**/
void _remove_rule_instance(rule_instance* ri, rule_queue* rq){
  unsigned int i;
  bool moveup = false;
  assert(rq->n_queue > 0);

  rq->n_queue--;
  if(rq->end == 0)
    rq->end = rq->size_queue-1;
  else
    rq->end--;
  for(i = rq->first; i != rq->end; i = (i+1) % rq->size_queue){
    if(ri == rq->queue[i])
      moveup = true;
    if(moveup)
      rq->queue[i] = rq->queue[(i+1) % rq->size_queue];
  }

  assert(moveup || ri == rq->queue[rq->end]);
}


bool test_rule_instance(const rule_instance* ri, const rete_net_state* state){
  assert(ri != NULL);
  assert(ri->rule != NULL);
  assert(ri->substitution != NULL);
  assert(test_substitution(ri->substitution));
  assert(test_axiom(ri->rule, ri->rule->axiom_no));
  if(ri->timestamp > get_latest_global_step_no(state)){
    fprintf(stderr, "Wrong timestamp %i on rule instance\n", ri->timestamp);
    return false;
  }  
  if(!test_is_instantiation(ri->rule->rhs->free_vars, ri->substitution)){
    fprintf(stderr, "An incorrect rule instance added to the queue\n");
    print_rule_instance(ri, stderr);
    return false;
  }
  return true;
}


/**
   External function for creating a rule instance
   
   Only used when commandline option fact-set is on, 
   that is, when RETE is not used
**/
rule_instance* create_rule_instance(const axiom* rule, substitution* sub){
  rule_instance* ins = malloc_tester(sizeof(rule_instance));

  assert(test_substitution(sub));
  
  // The timestamp on rule instances is only used by RETE. Otherwise, rule instances are immediately added to the factset
  ins->timestamp = 0;
  ins->rule = rule;
  ins->substitution = sub;
  return ins;
}

/**
   Used by prover as a stack marker in disj_ri_stack
**/
rule_instance* create_dummy_rule_instance(void){
  rule_instance* ins = malloc_tester(sizeof(rule_instance));
  ins->timestamp = 0;
  ins->rule = NULL;
  ins->substitution = NULL;
  ins->used_in_proof = false;
  return ins;
}
  
void delete_dummy_rule_instance(rule_instance* ri){
  free(ri);
}


/**
   External function for adding and popping instances of a rule
   to/from the axiom queue and the normal queueu

   The substitution is stored in a rule queue in the state, and deleted upon popping.
   The calling function must not touch sub after passing it to this function
**/
void add_rule_to_queue(const axiom* rule, substitution* sub, rete_net_state* state){
  rule_instance* ins = malloc_tester(sizeof(rule_instance));

  assert(test_substitution(sub));
  
  ins->timestamp = get_current_state_step_no(state);
  ins->rule = rule;
  ins->substitution = sub;
  ins->used_in_proof = false;

  assert(rule->axiom_no < state->net->th->n_axioms);
  assert(test_rule_instance(ins, state));

  assert(rule->axiom_no < state->net->th->n_axioms);
  assert(state->axiom_inst_queue[rule->axiom_no] != NULL);

  state->axiom_inst_queue[rule->axiom_no] = _add_rule_to_queue(ins, state->axiom_inst_queue[rule->axiom_no], state->net->strat == clpl_strategy);
}
rule_instance* _peek_rule_queue(const rule_queue* rq){
  assert(rq->n_queue > 0);  
  return rq->queue[rq->first];
}

rule_instance* peek_axiom_rule_queue(const rete_net_state* state, size_t axiom_no){
  rule_queue* rq = state->axiom_inst_queue[axiom_no];
  assert(axiom_no < state->net->th->n_axioms);
  return _peek_rule_queue(rq);
}

/**
   Called from prover when using CL.pl emulation mode

   Uses the rule queue as a stack, by taking the most recently added rule instance
**/
rule_instance* pop_youngest_axiom_rule_queue(rete_net_state* state, size_t axiom_no){
  rule_queue* rq = state->axiom_inst_queue[axiom_no];
  assert(axiom_no < state->net->th->n_axioms);

  rq->n_appl ++;
  rq->previous_appl = get_current_state_step_no(state);

  rule_instance* retval = _pop_youngest_rule_queue(rq);

  assert(test_rule_instance(retval, state));

  return retval;
}
  
rule_instance* pop_axiom_rule_queue(rete_net_state* state, size_t axiom_no){
  rule_queue* rq = state->axiom_inst_queue[axiom_no];
  assert(axiom_no < state->net->th->n_axioms);
  assert(rq->n_queue > 0);

  rq->n_appl++;
  rq->previous_appl = get_current_state_step_no(state);

  rule_instance* retval = _pop_rule_queue(rq);

  assert(test_rule_instance(retval, state));

  return retval;
}
  

/**
   Called from detrect_rete_beta_sub in prover.c

   Based on pointer identity of substitutions

   Will return NULL if there is no rule instance with substitution sub. 
   Therefore more complex and different than "_remove_rule_instance" above

   In profiling (gprof, 40k steps with COM008+1.1), uses, accumulated, 0.1 % of prover time
**/
rule_instance* _remove_substitution_rule_instance(rule_queue* rq, const substitution* sub){
  unsigned int i;
  bool moveup = false;
  rule_instance* retval = NULL;
  assert(test_substitution(sub));

#ifndef NDEBUG
  size_t size_before = rq->n_queue;
  unsigned int found_i = -1;
#endif

  if(rq->n_queue > 0){
    for(i = rq->first; i != rq->end; i = (i+1) % rq->size_queue){
      if(sub == rq->queue[i]->substitution){
	retval = rq->queue[i];
	assert(retval != NULL);
	moveup = true;
	rq->n_queue--;
#ifndef NDEBUG
	found_i = i;
#endif
	if(rq->end == 0)
	  rq->end = rq->size_queue-1;
	else
	  rq->end--;
      }
      if(moveup){
	rq->queue[i] = rq->queue[(i+1) %  rq->size_queue];
	assert(i == found_i || sub != rq->queue[i]->substitution);
      }
      if(i==rq->end)
	break;
    }
    assert(moveup  || retval == NULL || sub == rq->queue[rq->end]->substitution);
  }
  assert(( retval == NULL && size_before == rq->n_queue) || (size_before == rq->n_queue + 1));
  return retval;
}

void remove_rule_instance(rete_net_state* state, const substitution* sub, size_t axiom_no){
  assert(axiom_no < state->net->th->n_axioms);
#ifndef NDEBUG
  size_t size_before = state->axiom_inst_queue[axiom_no]->n_queue;
#endif

  rule_instance* remd = _remove_substitution_rule_instance(state->axiom_inst_queue[axiom_no], sub);

  if(remd != NULL){
    assert(size_before == state->axiom_inst_queue[axiom_no]->n_queue + 1);
  } else 
    assert(size_before == state->axiom_inst_queue[axiom_no]->n_queue);
}

/**
   Called from delete_full_rete_state in rete_state.c
   Only used when prover in prover.c is at the end
**/
void delete_full_rule_queue(rule_queue* rq){
  unsigned int i;
  for(i = rq->first; i != rq->end; i = (i+1) % rq->size_queue){
    free(rq->queue[i]);
  }
  free(rq);
}

/**
   Printing
**/
void print_rule_queue(const rule_queue* rq, FILE* f){
  unsigned int i, j;
  fprintf(f, "Rule queue %zi entries: \n", rq->n_queue);
  assert(rq->end == (rq->first + rq->n_queue) % rq->size_queue);
  j=1;
  for(i = rq->first; i < rq->end; i = (i+1) % rq->size_queue){
    fprintf(f, "\t%i: ", j++);
    print_rule_instance(rq->queue[i], f);
    fprintf(f, "\n");
  }
}
    

void print_dot_rule_queue(const rule_queue* rq, FILE* f){
  unsigned int i;
  fprintf(f, "Rule queue:{ ");
  assert(rq->end == (rq->first + rq->n_queue) % rq->size_queue);
  for(i = rq->first; i < rq->end; i = (i+1) % rq->size_queue){
    print_dot_rule_instance(rq->queue[i], f);
    fprintf(f, ", ");
  }
  fprintf(f, "}");
}
   
void print_coq_rule_instance(const rule_instance *ri, FILE* f){
  fprintf(f, "%s", ri->rule->name); 
  print_coq_substitution(ri->substitution, ri->rule->lhs->free_vars, f);
}


void print_dot_rule_instance(const rule_instance *ri, FILE* f){
  fprintf(f, " %s (", ri->rule->name); 
  print_dot_axiom(ri->rule, f);
  fprintf(f, ") ");
  print_substitution(ri->substitution, f);
}

void print_rule_instance(const rule_instance *ri, FILE* f){
  fprintf(f, " %s (", ri->rule->name); 
  print_fol_axiom(ri->rule, f);
  fprintf(f, ") ");
  print_substitution(ri->substitution, f);
  if(ri->rule->is_existential){
    fprintf(f, " - existential: ");
    
  } else
    fprintf(f, " - definite");
}
