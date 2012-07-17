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
#include "rule_instance.h"
#include "error_handling.h"

#ifdef HAVE_PTHREAD
#include <pthread.h>
#endif



extern bool debug, verbose, proof, dot, text, pdf;

static FILE* dot_fp = NULL;
static FILE* coq_fp = NULL;

#ifdef HAVE_PTHREAD
static pthread_mutex_t dot_file_lock = PTHREAD_MUTEX_INITIALIZER;
#endif

static bool file_open, coq_file_open;

/**
   Used for prinitng debugging messsages in prover.c
**/
FILE* get_coq_fdes(){
  assert(coq_fp != NULL);
  return coq_fp;
}

void end_proof_dot_writer(const char* filenameprefix, const rete_net* net){
  if(proof){ 
    char *dotfilename;
    dotfilename = malloc_tester(strlen(filenameprefix) + 6);
    sprintf(dotfilename, "%s.dot", filenameprefix);
#ifdef HAVE_PTHREAD
    pt_err(pthread_mutex_lock(& dot_file_lock), __FILE__, __LINE__, "Could not get lock on dot proof file\n");
#endif
    if(file_open){
      assert(dot_fp != NULL);
      fp_err( fprintf(dot_fp, "\n}\n"), "Could not write proof graph dot info\n");
      sys_err( fclose(dot_fp), "Could not close dot proof file\n");
      dot_fp = NULL;
      file_open = false;
    }
#ifdef HAVE_PTHREAD
    pt_err(pthread_mutex_unlock(&dot_file_lock),  __FILE__, __LINE__, "Could not release lock on dot proof file\n");
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
  char * coqfilename;
  coqfilename = malloc_tester(strlen(net->th->name) + 3);
  
  coqfilename = strcpy(coqfilename, net->th->name);
  
  coqfilename = strcat(coqfilename, ".v");
  printf("Writing coq format proof to file: \"%s\".\n", coqfilename);
  
  coq_fp = file_err( fopen(coqfilename, "w"), "Could not create proof coq .v file\n" );
  coq_file_open = true;

  print_coq_proof_intro(net->th, net->th->constants, coq_fp);
  
  free(coqfilename);
}

void write_proof_node(unsigned int step_no
		      , unsigned int cur_step_no
		      , const char* proof_branch_id
		      , const rete_net* net
		      , rule_queue_state rqs
		      , const constants* cs
		      , void (* print_new_facts)(rule_queue_state, FILE*)
		      , void (* print_rule_queues)(rule_queue_state, FILE*)
		      , const rule_instance* ri
		      )
{
#ifdef RETE_STATE_DEBUG_DOT
  FILE *fp2;
  char fname2[100];
#endif

  if(proof){
#ifdef HAVE_PTHREAD
    pt_err(pthread_mutex_lock(&dot_file_lock),  __FILE__, __LINE__, "Could not get lock on dot proof file\n");
#endif
    if(file_open){
      assert(dot_fp != NULL);
      assert(strlen(proof_branch_id) >= 0);
      fp_err( fprintf(dot_fp, "\n n%ss%i [label=\"%i",  
		      proof_branch_id,
		      step_no, 
		      cur_step_no
		      ), "Could not write proof node dot info\n");
#ifdef RETE_STATE_DEBUG_DOT
      fp_err( fprintf(dot_fp, "(Branch:%s:%i)", proof_branch_id, step_no), "Could not write proof node dot info\n");
#endif
      fp_err( fprintf(dot_fp, "\\n{" ), "Could not write proof node dot info\n");
      print_new_facts(rqs, dot_fp);
      fp_err( fprintf(dot_fp, "}\\n" ), "Could not write proof node dot info\n");
    } // file open
  } // proof etc. 
  if( verbose || debug){
    printf("In step: %i applied: ", cur_step_no);
    print_rule_instance(ri, cs, stdout);    
    printf("\n");
  } // proof etc. 

#ifdef RETE_STATE_DEBUG_TXT
  printf("\nRule Queues in step: %i: \n", cur_step_no);
  print_rule_queues(rqs, stdout);
  //  print_rule_queue(s->rule_queue, stdout);
#endif

  if(proof){
    if(file_open){
      print_dot_rule_instance(ri, cs, dot_fp);
      fp_err( fprintf(dot_fp, "\"] \n" ), "Could not write proof node dot info\n");
#ifdef RETE_STATE_DEBUG_DOT
      sprintf(fname2, "rete_state_%i.dot", cur_step_no);
      fp2 = file_err( fopen(fname2, "w"), "Could not create rete state dot file\n");
      print_dot_rete_state_net(net, rqs.single, fp2);
      fclose(fp2);
      sprintf(fname2, "dot -Tpdf rete_state_%i.dot > rete_state_%i.pdf && rm rete_state_%i.dot",  get_current_state_step_no(s), get_current_state_step_no(s), get_current_state_step_no(rqs.single));
      sys_err( system(fname2), "Could not execute dot on rete state file. Maybe dot is not installed?\n");
      
#endif
#ifdef HAVE_PTHREAD
      pt_err(pthread_mutex_unlock(&dot_file_lock),  __FILE__, __LINE__, "Could not release lock on dot proof file\n");
#endif
    }// file open
  }// proof || dot
}
void write_proof_edge(const char*  proof_branch_1, unsigned int step_no_1, const char* proof_branch_2, unsigned int step_no_2){
  if(proof){
    assert(dot_fp != NULL);
#ifdef HAVE_PTHREAD
    pt_err(pthread_mutex_lock(&dot_file_lock),  __FILE__, __LINE__, "Could not get lock on dot proof file\n");
#endif
    fp_err( fprintf(dot_fp, "\n n%ss%i -> n%ss%i \n", proof_branch_1, step_no_1, proof_branch_2, step_no_2 + 1 ), "Could not write proof node dot info\n");
#ifdef HAVE_PTHREAD
    pt_err(pthread_mutex_unlock(&dot_file_lock),  __FILE__, __LINE__,"Could not release lock on dot proof file\n");
#endif
  }
}

/**
   Coq proof output functions
**/

void write_exist_vars_intro(freevars* ev, const substitution* sub, const constants* cs){
  freevars_iter iter = get_freevars_iter(ev);
  while(has_next_freevars_iter(&iter)){
    clp_variable* var = next_freevars_iter(&iter);
    fp_err( fprintf(coq_fp, "intro "), "proof_writer.c: write_proof_node: Could not write to coq proof file.");
    print_coq_term(get_sub_value(sub, var->var_no), cs, coq_fp);
    fp_err( fprintf(coq_fp, ".\n"), "proof_writer.c: write_proof_node: Could not write to coq proof file.");
  }
}

/**
   Writes coq proof for starting a new branch in a disjunction

   ri is the rule instance with disjunction

   branch is the branch of the disjunction that is started. Indexed from 0 and up, increasing from left to right

   ts is the step where ri was first encountered
 **/
void write_disj_proof_start(const rule_instance* ri, timestamp ts, int branch, const constants* cs){
  assert(branch < ri->rule->rhs->n_args);
  fp_err( fprintf(coq_fp, "(* Proving branch %i from disj. split at step #%i*)\n", branch, ts.step), "proof_writer.c: write_proof_node: Could not write to coq proof file.");
  if(branch > 0 && branch < ri->rule->rhs->n_args - 1){
    fp_err( fprintf(coq_fp, "intro H_%i_%i.\n", ts.step, branch), "proof_writer.c: write_proof_node: Could not write to coq proof file.");
    fp_err( fprintf(coq_fp, "elim H_%i_%i.\n", ts.step, branch), "proof_writer.c: write_proof_node: Could not write to coq proof file.");
  }
  if(ri->rule->rhs->args[branch]->is_existential){
    freevars_iter iter = get_freevars_iter(ri->rule->rhs->args[branch]->bound_vars);
    unsigned int var_no = 0;
    fp_err( fprintf(coq_fp, "(* Introducing existential variables from branch %i of step %i*)\n", branch, ts.step), "proof_writer.c: write_proof_node: Could not write to coq proof file.");
    while(var_no++, has_next_freevars_iter(&iter)){
      clp_variable* var = next_freevars_iter(&iter);
      fp_err( fprintf(coq_fp, "intro H_%i_tmp_%i.\n", ts.step, var_no), "proof_writer.c: write_proof_node: Could not write to coq proof file.");
      fp_err( fprintf(coq_fp, "elim H_%i_tmp_%i.\n", ts.step, var_no), "proof_writer.c: write_proof_node: Could not write to coq proof file.");
      fp_err( fprintf(coq_fp, "intro "), "proof_writer.c: write_proof_node: Could not write to coq proof file.");
      print_coq_term(get_sub_value(& ri->sub, var->var_no), cs, coq_fp);
      fp_err( fprintf(coq_fp, ".\n"), "proof_writer.c: write_proof_node: Could not write to coq proof file.");
    }
    fp_err( fprintf(coq_fp, "intro H_%i.\n", ts.step), "proof_writer.c: write_proof_node: Could not write to coq proof file.");
  } else 
  fp_err( fprintf(coq_fp, "intro H_%i.\n", ts.step), "proof_writer.c: write_proof_node: Could not write to coq proof file.");
}

/**
   Called from check_used_rule_instances in prover.c
**/
void write_elim_usage_proof(const rete_net* net, rule_instance* ri, timestamp ts, const constants* cs){
  assert(ts.step > 0);
  if(net->coq){
    // Elimination of disjunction and existential variables is done on the way down.
    if(ri->rule->rhs->n_args > 1){
      fp_err( fprintf(coq_fp, "(* Disjunctive split at step %i*)\n", ts.step), "proof_writer.c: write_proof_node: Could not write to coq proof file.");
      fp_err( fprintf(coq_fp, "elim ("), "proof_writer.c: write_proof_node: Could not write to coq proof file.");
      print_coq_rule_instance(ri, cs, coq_fp);
      fp_err( fprintf(coq_fp, ").\n"), "proof_writer.c: write_proof_node: Could not write to coq proof file.");
      write_disj_proof_start(ri, ts, 0, cs);
    } else if(ri->rule->rhs->n_args == 1 && ri->rule->rhs->args[0]->is_existential){
      // Printing the elim part
      fp_err( fprintf(coq_fp, "(* Introducing existential variables from step %i *)\n", ts.step), "proof_writer.c: write_proof_node: Could not write to coq proof file.");
      fp_err( fprintf(coq_fp, "elim ("), "proof_writer.c: write_proof_node: Could not write to coq proof file.");
      print_coq_rule_instance(ri, cs, coq_fp);
      fp_err( fprintf(coq_fp, ").\n"), "proof_writer.c: write_proof_node: Could not write to coq proof file.");
      write_exist_vars_intro(ri->rule->rhs->args[0]->bound_vars, & ri->sub, cs);
      fp_err( fprintf(coq_fp, "intro H_%i.\n", ts.step), "proof_writer.c: write_proof_node: Could not write to coq proof file.");
    }
  }
}

const char* get_tactic_name(timestamp ts){
  if(is_normal_timestamp(ts))
    return "apply";
  else if(is_inv_rewrite_timestamp(ts))
    return "rewrite <-";
  else
    return "rewrite";
}

/**
   Called from run_prover when seeing a goal rule instance
**/
void write_goal_proof_step(const rule_instance* ri, const rete_net* net, timestamp ts, rule_instance * (* get_history)(unsigned int, rule_queue_state), rule_queue_state rqs, const constants* cs){  
  
  if(ri->rule->type == fact && ri->rule->rhs->n_args <= 1 && !ri->rule->is_existential){
    fp_err( fprintf(coq_fp, "(* Using fact #%i*)\n", ri->rule->axiom_no), "proof_writer.c: write_proof_node: Could not write to coq proof file.");
    fp_err( fprintf(coq_fp, "%s %s.\n", get_tactic_name(ts), ri->rule->name), "proof_writer.c: write_proof_node: Could not write to coq proof file.");
  } else {
    if(ri->rule->rhs->n_args > 1 || ri->rule->is_existential){
      fp_err( fprintf(coq_fp, "(* Applying from step #%i*)\n", ts.step), "proof_writer.c: write_proof_node: Could not write to coq proof file.");
      fp_err( fprintf(coq_fp, "%s H_%i.\n", get_tactic_name(ts), ts.step), "proof_writer.c: write_proof_node: Could not write to coq proof file.");
    } else {
      write_goal_proof(ri, net, ts, get_history, rqs, cs);
    }
  } /* end if not existential or disjunction */ 
}


void write_premiss_proof(const rule_instance* ri, timestamp ts, const rete_net* net, rule_instance * (* get_history)(unsigned int, rule_queue_state), rule_queue_state rqs, const constants* cs){
  unsigned int i = 1;
  timestamps_iter iter = get_sub_timestamps_iter(&ri->sub);
  while(has_next_timestamps_iter(&iter)){
    timestamp premiss_no = get_next_timestamps_iter(&iter);
    if(!is_equality_timestamp(premiss_no) && has_next_timestamps_iter(&iter))
      fp_err( fprintf(coq_fp, "split.\n"), "proof_writer.c: write_proof_node: Could not write to coq proof file.");
    if (is_reflexivity_timestamp(ts))
      fp_err( fprintf(coq_fp, "reflexivity.\n"), "proof_writer.c: write_proof_node: Could not write to coq proof file.");
    else if(is_init_timestamp(premiss_no)){
      fp_err( fprintf(coq_fp, "assumption.\n"), "proof_writer.c: write_proof_node: Could not write to coq proof file.");
    } else if(premiss_no.step != ts.step) {
      rule_instance* premiss = get_history(premiss_no.step, rqs);
      //if(!(a->type == fact && a->rhs->n_args == 1 && a->rhs->args[0]->n_args == 1 && a->rhs->args[0]->args[i]->pred->is_domain)){
      fp_err( fprintf(coq_fp, "(* Proving conjunct %i of step %i *)\n", i, ts.step), "proof_writer.c: write_proof_node: Could not write to coq proof file.");
      write_goal_proof_step(premiss, net, premiss_no, get_history, rqs, cs);
    }
    if(!is_equality_timestamp(premiss_no))
      i++;
  } // end while
  destroy_timestamps_iter(&iter);
}

void write_goal_proof(const rule_instance* ri, const rete_net* net, timestamp ts, rule_instance * (* get_history)(unsigned int, rule_queue_state), rule_queue_state rqs, const constants* cs){
  if(net->coq){
    fp_err( fprintf(coq_fp, "(* Applying fact from step %i *)\n", ts.step), "proof_writer.c: write_proof_node: Could not write to coq proof file.");
    fp_err( fprintf(coq_fp, "%s (", get_tactic_name(ts)), "proof_writer.c: write_proof_node: Could not write to coq proof file.");
    print_coq_rule_instance(ri, cs, coq_fp);
    fp_err( fprintf(coq_fp, ").\n"), "proof_writer.c: write_proof_node: Could not write to coq proof file.");
    write_premiss_proof(ri, ts, net, get_history, rqs, cs);
  }
}
  

