/* main.c

   Copyright 2011 ?

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
#include "theory.h"
#include "fresh_constants.h"
#include "rete.h"
#include "proof_writer.h"
#include "parser.h"
#include "strategy.h"
#include <unistd.h>
#include <getopt.h> 
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <time.h>

/**
   The boolean lazy leads to the opposite value for propagate in the 
   alpha nodes representing atoms on the left hand side. It first sets 
   the value of net->lazy, then this is used once, in the function create_rete_axiom_node
   in axiom.c
**/
bool lazy;

extern char * optarg;
extern int optind, optopt, opterr;
bool use_substitution_store;
bool verbose, debug, proof, text, existdom, factset_lhs, coq, multithreaded, use_beta_not, print_model, all_disjuncts;
strategy strat;
unsigned long maxsteps;
typedef enum input_format_type_t {clpl_input, geolog_input} input_format_type;
input_format_type input_format;

/**
   Extracts an non-negative integer commandline argument option
**/
unsigned long int get_ui_arg_opt(char * tailptr){
  errno = 0;
  unsigned long int retval  = strtoul(optarg, &tailptr, 0);
  if(tailptr[0] != '\0' || errno != 0){
    perror("Error with argument to option \"max\". Must be a positive integer stating maximum number of steps in the proof.\n");
    exit(EXIT_FAILURE);
  }
  return retval;
}

/**
   This is the function that is called when the cpu or wall timer expires.
   Used in start_timer below
**/
void timer_expired(union sigval notify_data){
  switch(notify_data.sival_int){
  case CLOCK_REALTIME:
    printf("The wall-clock-time expired. You may rerun with higher --wallclocktime (-w).\n");
    break;
  case CLOCK_PROCESS_CPUTIME_ID:
    printf("The cpu time expired. You may rerun with higher --cputime (-T).\n");
    break;
  default:
    fprintf(stderr, "main.c: timer_expired: %i is not a valid timer clock type.\n",notify_data.sival_int);
    assert(false);
    break;
  }
  exit(EXIT_FAILURE);
}

/**
   Called from cpu_timer and wallclock-timer below
   Command line arguments T and w for timers
   Starts a timer 
**/
void start_timer(unsigned long int maxtimer, int timer_type){
  struct sigevent timer_event;
  struct timespec timeout;
  struct timespec zero_time;
  struct itimerspec timer_val;
  timer_t * timer = malloc_tester(sizeof(timer_t));

  timer_event.sigev_notify = SIGEV_THREAD;
  timer_event.sigev_notify_function = timer_expired;
  timer_event.sigev_notify_attributes = NULL;
  

  timer_event.sigev_value.sival_int = timer_type;
  timer_event.sigev_value.sival_ptr = NULL;
  
  if(timer_create(timer_type, &timer_event, timer) != 0){
    perror("main.c: file_prover: Could not create timer");
    exit(EXIT_FAILURE);
  }
  timeout.tv_sec = maxtimer;
  timeout.tv_nsec = 0;
  zero_time.tv_sec = 0;
  zero_time.tv_nsec = 0;
  timer_val.it_interval = zero_time;
  timer_val.it_value = timeout;
  
  if(timer_settime(*timer, 0, & timer_val, NULL) != 0){
    perror("main.c: file_prover: Could not set timer");
    exit(EXIT_FAILURE);
  } 
}

/**
   Called when argument --cputimer is seen
   Starts a timer based on cpu time used
**/
void start_cpu_timer(unsigned long int maxtimer){
  start_timer(maxtimer, CLOCK_PROCESS_CPUTIME_ID);
}
/**
   Called when argument --wallclocktimer is seen
   Starts a timer based on wall clock time used
**/
void start_wallclock_timer(unsigned long int maxtimer){
    start_timer(maxtimer, CLOCK_REALTIME);
}

int file_prover(FILE* f, const char* prefix){
  theory* th;
  rete_net* net;
  FILE *fp;
  int retval;
  unsigned int steps;
  void * mem_break = sbrk(0);
  switch(input_format){
  case clpl_input:
    th = clpl_parser(f);
    break;
  case geolog_input:
    th = geolog_parser(f);
    break;
  default:
    perror("Error in enum for file input type\n");
    exit(EXIT_FAILURE);
  }

  assert(test_theory(th));
  if(!has_theory_name(th))
    set_theory_name(th, prefix);
  net = create_rete_net(th, maxsteps, existdom, strat, lazy, coq, use_beta_not, factset_lhs, print_model, all_disjuncts);

  if(!factset_lhs){  
    if(debug){
      fp = fopen("rete.dot", "w");
      if(fp == NULL){
	perror("file_prover: Could not open \"rete.dot\" for writing\n");
	exit(EXIT_FAILURE);
      } else {
	print_dot_rete_net(net, fp);
	if(fclose(fp) != 0)
	  perror("file_prover: Could not close \"rete.dot\"\n");
      }
    }
    assert(test_rete_net(net));
    
    init_proof_dot_writer(prefix, net);
  }
  if(coq)
    init_proof_coq_writer(net);
  

  steps = prover_single(net, multithreaded);
  if(steps > 0){
    printf("Found a proof after %i steps that the theory has no model\n", steps);
    retval = EXIT_SUCCESS;
  } else {
    if(verbose)
      printf("Could not find proof\n");
  }
  if(!factset_lhs){
    end_proof_dot_writer(prefix, net);
  }
  
  delete_rete_net(net);
    
  delete_theory(th);
  //if(brk(mem_break) != 0)
  //  perror("Error with resetting memory after proving file\n");

  return retval;
}
void print_help(char* exec){
  printf("Usage: %s [OPTION...] [FILE...]\n\n", exec);
  printf("Reads a coherent theory given in the format supported by John Fisher's geolog prover <http://johnrfisher.net/GeologUI/index.html> or in the format supported by Marc Bezem and Dimitri Hendriks CL.pl <http://www.few.vu.nl/~diem/research/ht/#tool>. The strategy information in the latter format is not used, only the axioms. ");
  printf("The program searches for a model of the given theory, or a proof that there is no finite model of the theory. ");
  printf("If no file name is given, a single theory is read from standard input. ");
  printf("Several file names can be given, each will then be parsed as a single theory and a proof search done for each of them.\n\n");
  printf(" Explanation of options:\n");
  printf("\t-p, --proof\t\tOutput a proof if one is found\n");
  printf("\t-g, --debug \t\tGives (lots of) extra output useful for debugging or understanding the prover\n");
  printf("\t-t, --text\t\tGives output of proof in separate text file. Same prefix of name as input file, but with .out as suffix. \n");
  printf("\t-v, --verbose\t\tGives extra output about the proving process\n");
  printf("\t-V, --version\t\tSome info about the program, including copyright and license\n");
  printf("\t-e, --eager\t\tUses the eager version of RETE. This is probably always slower.\n");
  printf("\t-q, --coq\t\tOutputs coq format proof to a file in the current working directory.\n");
  printf("\t-d, --depth-first\t\tUses a depth-first strategy, similar to in CL.pl. \n");
  printf("\t-f, --factset_lhs\t\tUses standard fact-set method to determine whether the lhs of an instance is satisified. The default is to use rete. Implies -n|--not. Work in progress.\n");
  printf("\t-o, --print_model\t\tPrints the model in the case that there is no proof of contradiction. (Implied by factset_lhs.)\n");
  printf("\t-m, --max=LIMIT\t\tMaximum number of inference steps in the proving process. 0 sets no limit\n");
  printf("\t-C, --CL.pl\t\tParses the input file as in CL.pl. This is the default.\n");
  printf("\t-G, --geolog\t\tParses the input file as in Fisher's geolog.See http://johnrfisher.net/GeologUI/index.html#geolog for a description\n");
  printf("\t-M, --multithreaded\t\tUses a multithreaded alogithm. Should probably be used with -a|--all-disjuncts.\n");
  printf("\t-T, --cpu_timer=LIMIT\t\tSets a limit to total number of seconds of CPU time spent.\n");
  printf("\t-w, --wallclocktimer=LIMIT\t\tSets a limit to total number of seconds that may elapse before prover exits.\n");
  printf("\t-a, --all-disjuncts\t\tAlways treats all disjuncts of all treated disjuncts. .\n");
  printf("\t-n, --not\t\tDo not constructs rete nodes for the rhs of rules, but in stead use a factset to determine whether the right hand side of a new instance is already satisified. (Beta-not-nodes).\n");
  printf("\t-s, --substitution_malloc\t\tUse a single malloc for each substitution. (Disables substitution_memory.c) \n");
  printf("\nReport bugs to <hovlanddag@gmail.com>\n");
}


void print_version(){
  printf("clp (coherent logic prover) 0.10\n"); 
  printf("Copyright (C) 2011 University of Oslo and/or Dag Hovland.\n");
  printf("This is free software.  You may redistribute copies of it under the terms of\n");
  printf("the GNU General Public License <http://www.gnu.org/licenses/gpl.html>.\n");
  printf("There is NO WARRANTY, to the extent permitted by law.\n\n");
  printf("Written by Dag Hovland.\n");
}

int main(int argc, char *argv[]){
  theory* th;
  rete_net* net;
  FILE* fp;
  int curopt;
  int retval = EXIT_FAILURE;
  const struct option longargs[] = {{"factset", no_argument, NULL, 'f'}, {"version", no_argument, NULL, 'V'}, {"verbose", no_argument, NULL, 'v'}, {"proof", no_argument, NULL, 'p'}, {"help", no_argument, NULL, 'h'}, {"debug", no_argument, NULL, 'g'}, {"factset_lhs", no_argument, NULL, 'f'}, {"text", no_argument, NULL, 't'},{"max", required_argument, NULL, 'm'}, {"depth-first", no_argument, NULL, 'd'}, {"eager", no_argument, NULL, 'e'}, {"multithreaded", no_argument, NULL, 'M'}, {"CL.pl", no_argument, NULL, 'C'}, {"geolog", no_argument, NULL, 'G'}, {"coq", no_argument, NULL, 'q'}, {"factset_rhs", no_argument, NULL, 's'}, {"not", no_argument, NULL, 'n'}, {"cputimer", required_argument, NULL, 'T'}, {"wallclocktimer", required_argument, NULL, 'w'}, {"print_model", no_argument, NULL, 'o'}, {"substitution_memory", no_argument, NULL, 's'}, {"all-disjuncts", no_argument, NULL, 'a'}, {0,0,0,0}};
  char shortargs[] = "w:vfVphgdoacCsaT:GeqMnm:";
  int longindex;
  char argval;
  char * tailptr;
  verbose = false;
  proof = false;
  text = false;
  debug = false;
  print_model = false;
  lazy = true;
  coq = false;
  use_beta_not = true;
  multithreaded = false;
  factset_lhs = false;
  all_disjuncts = false;
  use_substitution_store = true;
  input_format = clpl_input;
  strat = normal_strategy;
  maxsteps = MAX_PROOF_STEPS;

  while( ( argval = getopt_long(argc, argv, shortargs, &longargs[0], &longindex )) != -1){
    switch(argval){
      unsigned long int maxtimer;
    case 'v':
      verbose = true;
      break;
    case 'p':
      proof = true;
      break;
    case 's':
      use_substitution_store = false;
      break;
    case 'd':
      strat = clpl_strategy;
      break;
    case 'q':
      coq = true;
      break;
    case 'T':
      maxtimer = get_ui_arg_opt(tailptr);
      start_cpu_timer(maxtimer);
      break;
    case 'a':
      all_disjuncts = true;
      break;
    case 'w':
      maxtimer = get_ui_arg_opt(tailptr);
      start_wallclock_timer(maxtimer);
      break;
    case 'f':
      factset_lhs = true;
      break;
    case 'n':
      use_beta_not = false;
      break;
    case 'g':
      debug = true;
      break;
    case 'o':
      print_model = true;
      break;
    case 'e':
      lazy = false;
      break;
    case 'h':
      print_help(argv[0]);
      exit(EXIT_SUCCESS);
      break;
    case 'V':
      print_version();
      exit(EXIT_SUCCESS);
    case 'M':
      multithreaded = true;
      break;
    case 't':
      text = true;
      break;
    case 'C':
      input_format = clpl_input;
      break;
    case 'G':
      input_format = geolog_input;
      break;
    case 'm':
      maxsteps = get_ui_arg_opt(tailptr);
      break;
    default:
      fprintf(stderr, "Try '%s --help' for more information.\n", argv[0]);
      exit(EXIT_FAILURE);
    }
  } 
  if(optind < argc){
    retval = EXIT_SUCCESS;
    while( optind < argc ) {
      FILE* f = fopen(argv[optind], "r");
      if(f == NULL){
	perror("main: Could not open theory file");
	fprintf(stderr, "%s\n", argv[optind]); 
	exit(EXIT_FAILURE);
      }
      printf("%s\n", argv[optind]);

      if(file_prover(f, basename(argv[optind])) == EXIT_FAILURE)
	retval = EXIT_FAILURE;

      
      if(fclose(f) != 0){
	perror("main(): Could not close theory file\n");
	fprintf(stderr, "File name: %s\n", argv[optind]); 
	exit(EXIT_FAILURE);
      }
      optind++;
    } // end for
    } else {
  //    assert(optind == argc);
    retval =  file_prover(stdin, "STDIN");
  }

  return retval;
}

