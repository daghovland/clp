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

#ifdef HAVE_PTHREAD
#include <pthread.h>
#endif

#include <errno.h>

extern bool debug, verbose, proof, dot, text, pdf;

static FILE* dot_fp = NULL;

#ifdef HAVE_PTHREAD
static pthread_mutex_t dot_file_lock = PTHREAD_MUTEX_INITIALIZER;
#endif

static bool file_open;

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
   Wrapper for calls like, system, 
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

void end_proof_dot_writer(const char* filenameprefix){
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
      if (fclose(dot_fp) != 0){
	perror("Could not close dot proof file\n");
	exit(EXIT_FAILURE);
      }
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
}

void init_proof_dot_writer(const char* filenameprefix){
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
		      get_global_step_no(s)
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
    printf("In step: %i applied: ", get_global_step_no(s));
    print_rule_instance(ri, stdout);    
    printf("\n");
  } // proof etc. 

#ifdef RETE_STATE_DEBUG_TXT
  printf("\nRule Queues in step: %i: \n", get_global_step_no(s));
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
      sprintf(fname2, "rete_state_%i.dot", get_global_step_no(s));
      fp2 = file_err( fopen(fname2, "w"), "Could not create rete state dot file\n");
      print_dot_rete_state_net(s->net, s, fp2);
      fclose(fp2);
      sprintf(fname2, "dot -Tpdf rete_state_%i.dot > rete_state_%i.pdf && rm rete_state_%i.dot",  get_global_step_no(s), get_global_step_no(s), get_global_step_no(s));
      sys_err( system(fname2), "Could not execute dot on rete state file. Maybe dot is not installed?\n");
      
#endif
#ifdef HAVE_PTHREAD
      pt_err(pthread_mutex_unlock(&dot_file_lock), "Could not release lock on dot proof file\n");
#endif
    }// file open
  }// proof || dot

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
