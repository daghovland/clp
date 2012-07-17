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
  unsigned int i, timestamp = 0;
  if(rq == NULL || rq->queue == NULL){
    fprintf(stderr, "NULL queue\n");
    return false;
  }
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
    if(!test_rule_instance(rq->queue[i])){
      fprintf(stderr, "Invalid rule instance at queue position %i\n", i);
      return false;
    }
  } // end for(i = ...)
  return true;
}

/**
   Internal function for adding a rule instance to a queue

   The queue must be returned, since realloc might change the address
**/
rule_queue* _add_rule_to_queue(rule_instance* ri, rule_queue* rq, bool clpl_sorted){
  size_t orig_size = rq->size_queue;

  assert(!ri->used_in_proof);

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
    while(i != rq->first && compare_sub_timestamps(& rq->queue[j]->sub, & ri->sub) > 0){
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
/**
   Used by normal_next_instance. Pops the rule instance
   that was pushed most recently. 

   Must copy disjunction instances, so as not to mess up 
   the use in different branches

   The other instances are copied in the prover, when "used_in_proof" is set to true.
**/
rule_instance* _pop_rule_queue(rule_queue* rq, substitution_store_mt* store, substitution_size_info ssi){
  rule_instance* retval;

  assert(rq->n_queue > 0);
  assert(rq->first != rq->end);

  rq->n_queue--;
  retval =  rq->queue[rq->first++];
  rq->first %= rq->size_queue;

  assert(!retval->used_in_proof);


  retval = copy_rule_instance(retval, ssi);
  return retval;
}

/**
   Takes most recently added instance from rule queue

   Used by CL.pl emulation to choose next instance

   Must copy disjunction instances, so as not to mess up 
   the use in different branches

   The other rule instances are copied in prover.c, if they are seen to be used
   (That is, if the "used_in_proof" is set to true.)
**/
rule_instance* _pop_youngest_rule_queue(rule_queue* rq, substitution_store_mt* store, substitution_size_info ssi){
  rule_instance* retval;

  assert(rq->n_queue > 0);
  assert(rq->first != rq->end);

  rq->n_queue--;
  if(rq->end == 0)
    rq->end = rq->size_queue-1;
  else
    rq->end--;

  retval =  rq->queue[rq->end];

  assert(!retval->used_in_proof);
  
  retval = copy_rule_instance(retval, ssi);
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


unsigned int axiom_queue_previous_application(rule_queue_state rqs, unsigned int axiom_no){
  return (rqs.state)->axiom_inst_queue[axiom_no]->previous_appl;
}

void add_rule_to_queue_state(const clp_axiom* rule, const substitution* sub, rule_queue_state rqs){
  add_rule_to_queue(rule, sub, rqs.state);
}

/**
   External function for adding and popping instances of a rule
   to/from the axiom queue and the normal queueu

   The substitution is stored in a rule queue in the state, and deleted upon popping.
   The calling function must not touch sub after passing it to this function
**/
void add_rule_to_queue(const clp_axiom* rule, const substitution* sub, rete_net_state* state){
  substitution_size_info ssi = state->net->th->sub_size_info;

  rule_instance* ins = malloc_tester(get_size_rule_instance(ssi));

  assert(test_substitution(sub));

  ins->timestamp = get_current_state_step_no(state);
  ins->rule = rule;
  copy_substitution_struct(& ins->sub, sub, ssi);
  ins->used_in_proof = false;

  assert(rule->axiom_no < state->net->th->n_axioms);
  assert(test_rule_instance(ins));

  assert(rule->axiom_no < state->net->th->n_axioms);
  assert(state->axiom_inst_queue[rule->axiom_no] != NULL);

  state->axiom_inst_queue[rule->axiom_no] = _add_rule_to_queue(ins, state->axiom_inst_queue[rule->axiom_no], state->net->strat == clpl_strategy);
}
rule_instance* _peek_rule_queue(const rule_queue* rq){
  assert(rq->n_queue > 0);  
  return rq->queue[rq->first];
}

rule_instance* peek_axiom_rule_queue_state(rule_queue_state rqs, unsigned int axiom_no){
  return  peek_axiom_rule_queue(rqs.state, axiom_no);
}

rule_instance* peek_axiom_rule_queue(rete_net_state* state, unsigned int axiom_no){
  rule_queue* rq = state->axiom_inst_queue[axiom_no];
  assert(axiom_no < state->net->th->n_axioms);
  return _peek_rule_queue(rq);
}

/**
   Called from prover when using CL.pl emulation mode

   Uses the rule queue as a stack, by taking the most recently added rule instance
**/
rule_instance* pop_youngest_axiom_rule_queue(rule_queue_state rqs, unsigned int axiom_no){
  rete_net_state* state = rqs.state;
  substitution_store_mt* store = & state->local_subst_mem;
  substitution_size_info ssi = state->net->th->sub_size_info;
  rule_queue* rq = state->axiom_inst_queue[axiom_no];
  assert(axiom_no < state->net->th->n_axioms);

  rq->n_appl ++;
  rq->previous_appl = get_current_state_step_no(state);

  rule_instance* retval = _pop_youngest_rule_queue(rq, store, ssi);

  assert(test_rule_instance(retval));

  return retval;
}
  
rule_instance* pop_axiom_rule_queue_state(rule_queue_state rqs, unsigned int axiom_no){
  return pop_axiom_rule_queue(rqs.state, axiom_no);
}


rule_instance* pop_axiom_rule_queue(rete_net_state* state, unsigned int axiom_no){
  substitution_store_mt* store = & state->local_subst_mem;
  substitution_size_info ssi = state->net->th->sub_size_info;
  rule_queue* rq = state->axiom_inst_queue[axiom_no];
  assert(axiom_no < state->net->th->n_axioms);
  assert(rq->n_queue > 0);

  rq->n_appl++;
  rq->previous_appl = get_current_state_step_no(state);

  rule_instance* retval = _pop_rule_queue(rq, store, ssi);

  assert(test_rule_instance(retval));

  return retval;
}
size_t size_axiom_rule_queue(rete_net_state* state, size_t axiom_no){
  assert(state->axiom_inst_queue[axiom_no]->n_queue >= 0);
  return state->axiom_inst_queue[axiom_no]->n_queue;
}

bool is_empty_axiom_rule_queue(rete_net_state* state, unsigned int axiom_no){
  return (size_axiom_rule_queue(state, axiom_no) == 0);
}

bool is_empty_axiom_rule_queue_state(rule_queue_state rqs, unsigned int axiom_no){
  return is_empty_axiom_rule_queue(rqs.state, axiom_no);
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
      if(sub == & rq->queue[i]->sub){
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
	assert(i == found_i || sub != & rq->queue[i]->sub);
      }
      if(i==rq->end)
	break;
    }
    assert(moveup  || retval == NULL || sub == & rq->queue[rq->end]->sub);
  }
  assert(( retval == NULL && size_before == rq->n_queue) || (size_before == rq->n_queue + 1));
  return retval;
}

void remove_rule_instance(rete_net_state* state, const substitution* sub, unsigned int axiom_no){
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
void print_rule_queue(const rule_queue* rq, const constants* cs, FILE* f){
  unsigned int i, j;
  fprintf(f, "queue with %zi entries: \n", rq->n_queue);
  assert(rq->end == (rq->first + rq->n_queue) % rq->size_queue);
  j=1;
  for(i = rq->first; i < rq->end; i = (i+1) % rq->size_queue){
    fprintf(f, "\t%i: ", j++);
    print_rule_instance(rq->queue[i], cs, f);
    fprintf(f, "\n");
  }
}
    

void print_dot_rule_queue(const rule_queue* rq, const constants* cs, FILE* f){
  unsigned int i;
  fprintf(f, "Rule queue:{ ");
  assert(rq->end == (rq->first + rq->n_queue) % rq->size_queue);
  for(i = rq->first; i < rq->end; i = (i+1) % rq->size_queue){
    print_dot_rule_instance(rq->queue[i], cs, f);
    fprintf(f, ", ");
  }
  fprintf(f, "}");
}
