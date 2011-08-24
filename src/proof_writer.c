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
history_array_lock
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

void end_proof_dot_writer(const char* filenameprefix, const rete_net* net){
  if(proof || dot || pdf){ 
    char *dotfilename, *pdffilename, *command;
    dotfilename = malloc_tester(strlen(filenameprefix) + 6);
    pdffilename = malloc_tester(strlen(filenameprefix) + 6);
    sprintf(dotfilename, "%s.dot", filenameprefix);
    sprintf(pdffilename, "%s.pdf", filenameprefix);
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
    if(pdf){
      command = malloc_tester(strlen(filenameprefix)*2 + 50);
      sprintf(command, "dot -Tpdf %s > %s", dotfilename, pdffilename);
      sys_err( system(command), 
	       "Could not execute dot on proof file. Maybe dot is not installed?\n");
      free(command);
    } // end if pdf
    free(dotfilename);
    free(pdffilename);
  } // if proof etc. 
  if(net->coq){
    assert(coq_file_open);
    print_coq_proof_ending(net->th, coq_fp);
    sys_err( fclose(coq_fp), "Could not close coq .v proof file.");
  }
}

void init_proof_dot_writer(const char* filenameprefix, const rete_net* net){
  if(proof || dot || pdf){
    char* dotfilename;
    dotfilename = malloc_tester(strlen(filenameprefix) + 5);
    sprintf(dotfilename, "%s.dot", filenameprefix);
    dot_fp = file_err( fopen(dotfilename, "w"), "Could not open proof dot file\n" );
    fp_err( fprintf(dot_fp, "strict digraph proof{\n"), "Could not write proof graph dot info\n");
    //    atexit(end_proof_dot_writer);
    file_open = true;
    free(dotfilename);
  }
  if(net->coq){
    char * coqfilename, * period_pos;
    unsigned int i;
    coqfilename = malloc_tester(strlen(filenameprefix) + 3);

    for(i = 0; filenameprefix[i] != '.' && filenameprefix[i] != '\0'; i++)
      coqfilename[i] = filenameprefix[i];
    coqfilename[i] = '\0';

    assert(i == strlen(coqfilename));

    coqfilename = strcat(coqfilename, ".v");
    printf("Writing coq format proof to file: \"%s\".\n", coqfilename);

    coq_fp = file_err( fopen(coqfilename, "w"), "Could not create proof coq .v file\n" );
    coq_file_open = true;
    print_coq_proof_intro(net->th, coq_fp);
    free(coqfilename);
  }
}

void write_proof_node(rete_net_state* s, const rule_instance* ri){
#ifdef RETE_STATE_DEBUG_DOT
  FILE *fp2;
  char fname2[100];
#endif
  unsigned int i;

  if(proof || pdf || dot){
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
      fp_err( fprintf(dot_fp, "\\n{" ), "Could not write proof node dot info\n");
      print_state_new_facts(s, dot_fp);
      fp_err( fprintf(dot_fp, "}\\n" ), "Could not write proof node dot info\n");
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
    if(s->axiom_inst_queue[i]->end != s->axiom_inst_queue[i]->first)
      print_rule_queue(s->axiom_inst_queue[i], stdout);
  }
  //  print_rule_queue(s->rule_queue, stdout);
#endif

  if(proof || dot || pdf){
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

  if(s->net->coq){
    rete_net* net = (rete_net*) s->net;
#ifdef HAVE_PTHREAD
    if(s->cursteps >= net->size_history){
      pt_err(pthread_mutex_lock(&history_array_lock), "Could not get lock on history array.\n");
#endif
      if(s->cursteps >= net->size_history - 1){
	net->size_history *= 2;
	net->history = realloc_tester(net->history, net->size_history);
      }
#ifdef HAVE_PTHREAD
      pt_err(pthread_mutex_unlock(&history_array_lock), "Could not release lock on history array.\n");
    } // end second if, avoiding unnecessary locking
#endif
    /**
       Note that s->cursteps is set by inc_proof_step_counter to the current global step that is worked on

       At the moment this leads to a memory crash, so it is disabled
    **/
    //net->history[s->cursteps] = ri;

    // Proofs written at the leafs?


    // Elimination of disjunction is done on the way.
    if(ri->rule->rhs->n_args > 1){
      unsigned int i, n_vars;
      n_vars = ri->rule->lhs->free_vars->n_vars;
      if(s->start_step_no > 0){
	fp_err( fprintf(coq_fp, "(* Proving a branch from step #%i*)\n", s->start_step_no), "proof_writer.c: write_proof_node: Could not write to coq proof file.");
	fp_err( fprintf(coq_fp, "intro H_or_%i.\n", s->start_step_no), "proof_writer.c: write_proof_node: Could not write to coq proof file.");
      }
      fp_err( fprintf(coq_fp, "(* Disjunctive split at step #%i*)\n", get_current_state_step_no(s)), "proof_writer.c: write_proof_node: Could not write to coq proof file.");
      fp_err( fprintf(coq_fp, "elim ("), "proof_writer.c: write_proof_node: Could not write to coq proof file.");
      print_coq_rule_instance(ri, coq_fp);
      fp_err( fprintf(coq_fp, ").\n"), "proof_writer.c: write_proof_node: Could not write to coq proof file.");
    }
  }
}

void write_proof_edge(const rete_net_state* s1, const rete_net_state* s2){
  if(proof || dot || pdf){
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
