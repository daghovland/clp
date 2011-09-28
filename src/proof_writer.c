/* proof_writer.c

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
/**
   Writes out the proof to a dot-file, or something else
**/

#include "common.h"
#include "proof_writer.h"
#include "rete.h"
#include "substitution.h"

#include <string.h>

#ifdef HAVE_PTHREAD
#include <pthread.h>
#endif

#include <errno.h>

extern bool debug, verbose, proof, dot, text, pdf;

static FILE* dot_fp = NULL;
static FILE* coq_fp = NULL;

#ifdef HAVE_PTHREAD
static pthread_mutex_t dot_file_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t history_array_lock = PTHREAD_MUTEX_INITIALIZER;

#endif

static bool file_open, coq_file_open;

#ifdef HAVE_PTHREAD
/**
   Wrapper for pthread function calls
**/
void pt_err(int errval, const char* msg){
  if(errval != 0){
    errno = errval;
    perror(msg);
    exit(EXIT_FAILURE);
  }
}
#endif
/**
   Wrapper for function calls like fprintf, which return 
   a negative value on error
**/
void fp_err(int retval, const char* msg){
  if(retval < 0){
    perror(msg);
    exit(EXIT_FAILURE);
  }
}

/**
   Wrapper for calls like, system and fclose
   which return 0 on success
**/
void sys_err(int retval, const char* msg){
  if(retval != 0){
    perror(msg);
    exit(EXIT_FAILURE);
  }
}


/**
   Checks for errors in calls that return FILE* or NULL on error, like fopen
**/
FILE* file_err(FILE* retval, const char* msg){
  if(retval == NULL)
    perror(msg);
  return retval;
}

/**
   Used for prinitng debugging messsages in prover.c
**/
FILE* get_coq_fdes(){
  assert(coq_fp != NULL);
  return coq_fp;
}

void end_proof_dot_writer(const char* filenameprefix, const rete_net* net){
  if(proof){ 
    char *dotfilename, *command;
    dotfilename = malloc_tester(strlen(filenameprefix) + 6);
    sprintf(dotfilename, "%s.dot", filenameprefix);
#ifdef HAVE_PTHREAD
    pt_err(pthread_mutex_lock(& dot_file_lock), "Could not get lock on dot proof file\n");
#endif
    if(file_open){
      assert(dot_fp != NULL);
      fp_err( fprintf(dot_fp, "\n}\n"), "Could not write proof graph dot info\n");
      sys_err( fclose(dot_fp), "Could not close dot proof file\n");
      dot_fp = NULL;
      file_open = false;
    }
#ifdef HAVE_PTHREAD
    pt_err(pthread_mutex_unlock(&dot_file_lock), "Could not release lock on dot proof file\n");
#endif  
    free(dotfilename);
  } // if proof etc. 
  
}

void end_proof_coq_writer(const theory* th){
  assert(coq_file_open);
  print_coq_proof_ending(th, coq_fp);
  sys_err( fclose(coq_fp), "Could not close coq .v proof file.");
}

void init_proof_dot_writer(const char* filenameprefix, const rete_net* net){
  if(proof){
    char* dotfilename;
    dotfilename = malloc_tester(strlen(filenameprefix) + 5);
    sprintf(dotfilename, "%s.dot", filenameprefix);
    dot_fp = file_err( fopen(dotfilename, "w"), "Could not open proof dot file\n" );
    fp_err( fprintf(dot_fp, "strict digraph proof{\n"), "Could not write proof graph dot info\n");
    //    atexit(end_proof_dot_writer);
    file_open = true;
    free(dotfilename);
  }
}

void init_proof_coq_writer(const rete_net* net){
  char *coqfileprefix, * coqfilename, * period_pos;
  unsigned int i;
  coqfilename = malloc_tester(strlen(net->th->name) + 3);
  
  coqfilename = strcpy(coqfilename, net->th->name);
  
  coqfilename = strcat(coqfilename, ".v");
  printf("Writing coq format proof to file: \"%s\".\n", coqfilename);
  
  coq_fp = file_err( fopen(coqfilename, "w"), "Could not create proof coq .v file\n" );
  coq_file_open = true;

  print_coq_proof_intro(net->th, coq_fp);
  
  free(coqfilename);
}

void write_proof_node(rete_net_state* s, const rule_instance* ri){
#ifdef RETE_STATE_DEBUG_DOT
  FILE *fp2;
  char fname2[100];
#endif
  unsigned int i;

  if(proof){
#ifdef HAVE_PTHREAD
    pt_err(pthread_mutex_lock(&dot_file_lock), "Could not get lock on dot proof file\n");
#endif
    if(file_open){
      assert(dot_fp != NULL);
      assert(strlen(s->proof_branch_id) >= 0);
      fp_err( fprintf(dot_fp, "\n n%ss%i [label=\"%i",  
		      s->proof_branch_id,
		      s->step_no, 
		      get_current_state_step_no(s)
		      ), "Could not write proof node dot info\n");
#ifdef RETE_STATE_DEBUG_DOT
      fp_err( fprintf(dot_fp, "(Branch:%s:%i)", s->proof_branch_id, s->step_no), "Could not write proof node dot info\n");
#endif
      if(s->net->has_factset){
	fp_err( fprintf(dot_fp, "\\n{" ), "Could not write proof node dot info\n");
	print_state_new_facts(s, dot_fp);
	fp_err( fprintf(dot_fp, "}\\n" ), "Could not write proof node dot info\n");
      }
    } // file open
  } // proof etc. 
  if( verbose || debug){
    printf("In step: %i applied: ", get_current_state_step_no(s));
    print_rule_instance(ri, stdout);    
    printf("\n");
  } // proof etc. 

#ifdef RETE_STATE_DEBUG_TXT
  printf("\nRule Queues in step: %i: \n", get_current_state_step_no(s));
  for(i = 0; i < s->net->th->n_axioms; i++){
    if(s->axiom_inst_queue[i]->end != s->axiom_inst_queue[i]->first){
      printf("Axiom %s ", s->net->th->axioms[i]->name);
      print_rule_queue(s->axiom_inst_queue[i], stdout);
    }
  }
  //  print_rule_queue(s->rule_queue, stdout);
#endif

  if(proof){
    if(file_open){
#ifdef RETE_STATE_DEBUG_DOT
      print_dot_rule_queue(s->rule_queue, dot_fp);
      fp_err( fprintf(dot_fp, "\\n" ), "Could not write proof node dot info\n");
#endif
      print_dot_rule_instance(ri, dot_fp);
      fp_err( fprintf(dot_fp, "\"] \n" ), "Could not write proof node dot info\n");
#ifdef RETE_STATE_DEBUG_DOT
      sprintf(fname2, "rete_state_%i.dot", get_current_state_step_no(s));
      fp2 = file_err( fopen(fname2, "w"), "Could not create rete state dot file\n");
      print_dot_rete_state_net(s->net, s, fp2);
      fclose(fp2);
      sprintf(fname2, "dot -Tpdf rete_state_%i.dot > rete_state_%i.pdf && rm rete_state_%i.dot",  get_current_state_step_no(s), get_current_state_step_no(s), get_current_state_step_no(s));
      sys_err( system(fname2), "Could not execute dot on rete state file. Maybe dot is not installed?\n");
      
#endif
#ifdef HAVE_PTHREAD
      pt_err(pthread_mutex_unlock(&dot_file_lock), "Could not release lock on dot proof file\n");
#endif
    }// file open
  }// proof || dot
}
void write_proof_edge(const rete_net_state* s1, const rete_net_state* s2){
  if(proof){
    assert(dot_fp != NULL);
#ifdef HAVE_PTHREAD
    pt_err(pthread_mutex_lock(&dot_file_lock), "Could not get lock on dot proof file\n");
#endif
    fp_err( fprintf(dot_fp, "\n n%ss%i -> n%ss%i \n", s1->proof_branch_id, s1->step_no, s2->proof_branch_id, s2->step_no+1 ), "Could not write proof node dot info\n");
#ifdef HAVE_PTHREAD
    pt_err(pthread_mutex_unlock(&dot_file_lock), "Could not release lock on dot proof file\n");
#endif
  }
}

/**
   Coq proof output functions
**/

void write_exist_vars_intro(freevars* ev, substitution* sub){
  freevars_iter iter = get_freevars_iter(ev);
  while(has_next_freevars_iter(&iter)){
    variable* var = next_freevars_iter(&iter);
    fp_err( fprintf(coq_fp, "intro "), "proof_writer.c: write_proof_node: Could not write to coq proof file.");
    print_coq_term(sub->values[var->var_no], coq_fp);
    fp_err( fprintf(coq_fp, ".\n"), "proof_writer.c: write_proof_node: Could not write to coq proof file.");
  }
}

/**
   Writes coq proof for starting a new branch in a disjunction

   ri is the rule instance with disjunction

   branch is the branch of the disjunction that is started. Indexed from 0 and up, increasing from left to right

   ts is the step where ri was first encountered
 **/
void write_disj_proof_start(const rule_instance* ri, int ts, int branch){
  assert(branch < ri->rule->rhs->n_args);
  fp_err( fprintf(coq_fp, "(* Proving branch %i from disj. split at step #%i*)\n", branch, ts), "proof_writer.c: write_proof_node: Could not write to coq proof file.");
  if(branch > 0 && branch < ri->rule->rhs->n_args - 1){
    fp_err( fprintf(coq_fp, "intro H_%i_%i.\n", ts, branch), "proof_writer.c: write_proof_node: Could not write to coq proof file.");
    fp_err( fprintf(coq_fp, "elim H_%i_%i.\n", ts, branch), "proof_writer.c: write_proof_node: Could not write to coq proof file.");
  }
  if(ri->rule->rhs->args[branch]->is_existential){
    fp_err( fprintf(coq_fp, "intro H_%i_tmp.\n", ts), "proof_writer.c: write_proof_node: Could not write to coq proof file.");
    fp_err( fprintf(coq_fp, "(* Introducing existential variables from branch %i of step %i*)\n", branch, ts), "proof_writer.c: write_proof_node: Could not write to coq proof file.");
    fp_err( fprintf(coq_fp, "elim H_%i_tmp.\n", ts), "proof_writer.c: write_proof_node: Could not write to coq proof file.");
    write_exist_vars_intro(ri->rule->rhs->args[branch]->bound_vars, ri->substitution);
    fp_err( fprintf(coq_fp, "intro H_%i.\n", ts), "proof_writer.c: write_proof_node: Could not write to coq proof file.");
  } else 
  fp_err( fprintf(coq_fp, "intro H_%i.\n", ts), "proof_writer.c: write_proof_node: Could not write to coq proof file.");
}

/**
   Called from check_used_rule_instances in prover.c
**/
void write_elim_usage_proof(rete_net_state* s, rule_instance* ri, int ts){
  assert(ts > 0);
  if(s->net->coq){
    // Elimination of disjunction and existential variables is done on the way down.
    if(ri->rule->rhs->n_args > 1){
      unsigned int i;

      fp_err( fprintf(coq_fp, "(* Disjunctive split at step %i*)\n", ts), "proof_writer.c: write_proof_node: Could not write to coq proof file.");
      fp_err( fprintf(coq_fp, "elim ("), "proof_writer.c: write_proof_node: Could not write to coq proof file.");
      print_coq_rule_instance(ri, coq_fp);
      fp_err( fprintf(coq_fp, ").\n"), "proof_writer.c: write_proof_node: Could not write to coq proof file.");
      write_disj_proof_start(ri, ts, 0);
    } else if(ri->rule->rhs->n_args == 1 && ri->rule->rhs->args[0]->is_existential){
      unsigned int i;
      // Printing the elim part
      fp_err( fprintf(coq_fp, "(* Introducing existential variables from step %i *)\n", ts), "proof_writer.c: write_proof_node: Could not write to coq proof file.");
      fp_err( fprintf(coq_fp, "elim ("), "proof_writer.c: write_proof_node: Could not write to coq proof file.");
      print_coq_rule_instance(ri, coq_fp);
      fp_err( fprintf(coq_fp, ").\n"), "proof_writer.c: write_proof_node: Could not write to coq proof file.");
      write_exist_vars_intro(ri->rule->rhs->args[0]->bound_vars, ri->substitution);
      fp_err( fprintf(coq_fp, "intro H_%i.\n", ts), "proof_writer.c: write_proof_node: Could not write to coq proof file.");
    }
  }
}

/**
   Called from run_prover when seeing a goal rule instance
**/
void write_goal_proof_step(const rule_instance* ri, const rete_net_state* state, int ts, rule_instance_state** history){  
  if(ri->rule->type == fact && ri->rule->rhs->n_args <= 1){
    const axiom* a = ri->rule;
    fp_err( fprintf(coq_fp, "(* Using fact #%i*)\n", ri->rule->axiom_no), "proof_writer.c: write_proof_node: Could not write to coq proof file.");
    fp_err( fprintf(coq_fp, "apply %s.\n", ri->rule->name), "proof_writer.c: write_proof_node: Could not write to coq proof file.");
  } else {
    if(ri->rule->rhs->n_args > 1 || ri->rule->is_existential){
      fp_err( fprintf(coq_fp, "(* Applying from step #%i*)\n", ts), "proof_writer.c: write_proof_node: Could not write to coq proof file.");
      fp_err( fprintf(coq_fp, "apply H_%i.\n", ts), "proof_writer.c: write_proof_node: Could not write to coq proof file.");
    } else {
      write_goal_proof(ri, state, ts, history);
    }
  } /* end if not existential or disjunction */ 
}


void write_premiss_proof(const rule_instance* ri, int ts, rule_instance_state** history){
  unsigned int i, n_premises = ri->substitution->n_timestamps;
  assert(n_premises == ri->rule->lhs->n_args);
  for(i = 0; i < n_premises; i++){
    int premiss_no = ri->substitution->timestamps[i];
    if(premiss_no > 0){
      rule_instance* premiss = history[premiss_no]->ri;
      const axiom* a = premiss->rule;
      //if(!(a->type == fact && a->rhs->n_args == 1 && a->rhs->args[0]->n_args == 1 && a->rhs->args[0]->args[i]->pred->is_domain)){
      fp_err( fprintf(coq_fp, "(* Proving conjunct #%i of step %i *)\n", i, ts), "proof_writer.c: write_proof_node: Could not write to coq proof file.");
      if(i < n_premises - 1){
	fp_err( fprintf(coq_fp, "split.\n"), "proof_writer.c: write_proof_node: Could not write to coq proof file.");
      }
      write_goal_proof_step(premiss, history[premiss_no]->s, premiss_no, history);
    }
  }
}

void write_goal_proof(const rule_instance* ri, const rete_net_state* state, int ts, rule_instance_state** history){
  if(state->net->coq){
    fp_err( fprintf(coq_fp, "(* Applying fact from step %i *)\n", ts), "proof_writer.c: write_proof_node: Could not write to coq proof file.");
    fp_err( fprintf(coq_fp, "apply ("), "proof_writer.c: write_proof_node: Could not write to coq proof file.");
    print_coq_rule_instance(ri, coq_fp);
    fp_err( fprintf(coq_fp, ").\n"), "proof_writer.c: write_proof_node: Could not write to coq proof file.");
    write_premiss_proof(ri, ts, history);
  }
}
  

