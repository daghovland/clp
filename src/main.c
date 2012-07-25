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
#include <stdio.h>
#include <assert.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
// For timer functions
#include <signal.h>
#include <time.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
// For JJParser
#include "DataTypes.h"
#include "Utilities.h"
#include "FileUtilities.h"
#include "Tokenizer.h"
#include "Parsing.h"
#include "Signature.h"
#include "Examine.h"
#include "Compare.h"
#include "Modify.h"
#include "List.h"
#include "Tree.h"
#include "ListStatistics.h"
#include "PrintTSTP.h"
#include "PrintDFG.h"
#include "PrintKIF.h"
#include "PrintOtter.h"
#include "PrintXML.h"
#include "SystemOnTPTP.h"

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
bool output_theory;
bool verbose, debug, proof, text, existdom, factset_lhs, coq, multithreaded, use_beta_not, print_model, all_disjuncts, dry_run, multithread_rete;
strategy strat;
unsigned long maxsteps;
typedef enum format_type_t {coherent_tptp_format, full_tptp_format, clpl_format, geolog_format} format_type;
format_type input_format;
format_type output_format;

/**
   Extracts an non-negative integer commandline argument option
**/
unsigned long int get_ui_arg_opt(){
  char *tailptr;
  errno = 0;
  unsigned long int retval  = strtoul(optarg, &tailptr, 0);
  if(tailptr[0] != '\0' || errno != 0){
    perror("Error with argument to option. Must be a positive integer stating maximum number of steps in the proof.\n");
    exit(EXIT_FAILURE);
  }
  return retval;
}

/**
   Extracts an character string commandline argument option
**/
format_type get_format_arg_opt(){
  format_type out;
  errno = 0;
  switch(optarg[0]){
  case 't':
  case 'T':
    out = coherent_tptp_format;
    break;
  case 'g':
  case 'G':
    out = geolog_format;
    break;
    break;
  case 'C':
  case 'c':
    out = geolog_format;
    break;
  default:
    perror("Error with format argument. Must be one of \"TPTP\", \"CL.pl\", or \"geolog\".\n");
    exit(EXIT_FAILURE);
  }
  return out;
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

int file_prover(FILE* f, const char* prefix, char* filename){
  theory* th;
  rete_net* net;
  FILE *fp;
  int retval;
  unsigned int steps;
  SIGNATURE Signature;
  LISTNODE Head;
  switch(input_format){
  case clpl_format:
    th = clpl_parser(f);
    break;
  case geolog_format:
    th = geolog_parser(f);
    break;
  case full_tptp_format: 
    
//----One signature for all testing
    Signature = NewSignature();

//----Read from file or stdin
    SetNeedForNonLogicTokens(0);
    SetAllowFreeVariables(0);
//----Test duplicate arity warnings
    SetWarnings(1);
    printf("Opening file\n");
    if(f == stdin)
      Head = ParseFILEOfFormulae(stdin, Signature,1,NULL);
    else {
      assert(f == NULL);
      Head = ParseFileOfFormulae(filename,NULL, Signature,1,NULL);
    }
    PrintListOfAnnotatedTSTPNodes(stdout,Signature,Head,tptp,1);
    return false;
  case coherent_tptp_format:
    th = tptp_parser(f);
    break;
  default:
    perror("Error in enum for file input type\n");
    exit(EXIT_FAILURE);
  }
  if(output_theory){
    switch(output_format){
    case full_tptp_format:
      assert(false);
      break;
    case coherent_tptp_format:
      print_tptp_theory(th, th->constants, stdout);
      break;
    case clpl_format:
      print_clpl_theory(th, th->constants, stdout);
      break;
    case geolog_format:
      print_geolog_theory(th, th->constants, stdout);
      break;
    default:
      perror("Unknown format. Cannot print.\n");
      exit(EXIT_FAILURE);
    }
  }
  assert(test_theory(th));
  if(!has_theory_name(th))
    set_theory_name(th, prefix);
  net = create_rete_net(th, maxsteps, existdom, strat, lazy, coq, use_beta_not, factset_lhs, print_model, all_disjuncts, verbose, multithread_rete);

  if(dry_run)
    exit(EXIT_SUCCESS);
  if(!factset_lhs){  
    if(debug){
      fp = fopen("rete.dot", "w");
      if(fp == NULL){
	perror("file_prover: Could not open \"rete.dot\" for writing\n");
	exit(EXIT_FAILURE);
      } else {
	print_dot_rete_net(net, th->constants, fp);
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
  //steps = prover(net, multithreaded);
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
  if(input_format == full_tptp_format){
    FreeListOfAnnotatedFormulae(&Head);
    FreeSignature(&Signature);
  }
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
  printf("\t-x, --text\t\tGives output of proof in separate text file. Same prefix of name as input file, but with .out as suffix. \n");
  printf("\t-v, --verbose\t\tGives extra output about the proving process\n");
  printf("\t-V, --version\t\tSome info about the program, including copyright and license\n");
  printf("\t-e, --eager\t\tUses the eager version of RETE. This is probably always slower.\n");
  printf("\t-q, --coq\t\tOutputs coq format proof to a file in the current working directory.\n");
  printf("\t-d, --depth-first\t\tUses a depth-first strategy, similar to in CL.pl. \n");
  printf("\t-f, --factset_lhs\t\tUses standard fact-set method to determine whether the lhs of an instance is satisified. The default is to use rete. Implies -n|--not. Work in progress.\n");
  printf("\t-P, --print=TPTP|Geolog|CL.pl\t\tOnly outputs the theory in the chosen format.\n");
  printf("\t-o, --print_model\t\tPrints the model in the case that there is no proof of contradiction. (Implied by factset_lhs.)\n");
  printf("\t-D, --dry-run\t\tPrevents running the prover proper. Only parses and constructs rete net, possibly outputs theory.\n");
  printf("\t-m, --max=LIMIT\t\tMaximum number of inference steps in the proving process. 0 sets no limit\n");
  printf("\t-C, --CL.pl\t\tParses the input file as in CL.pl. This is the default.\n");
  printf("\t-G, --geolog\t\tParses the input file as in Fisher's geolog.See http://johnrfisher.net/GeologUI/index.html#geolog for a description\n");
  printf("\t-T, --TPTP\t\tParses the input as TPTP simple, and translates to CL. See http://www.cs.miami.edu/~tptp/. Not finished. \n");
  printf("\t-M, --multithreaded\t\tUses a multithreaded algorithm. Should probably be used with -a|--all-disjuncts.\n");
  printf("\t-t, --cpu_timer=LIMIT\t\tSets a limit to total number of seconds of CPU time spent.\n");
  printf("\t-S, --single-threaded-rete\t\tPrevents the multithreaded rete implementation to run. Probably only interesting for testing.\n");
  printf("\t-w, --wallclocktimer=LIMIT\t\tSets a limit to total number of seconds that may elapse before prover exits.\n");
  printf("\t-a, --all-disjuncts\t\tAlways treats all disjuncts of all treated disjuncts.\n");
  printf("\t-n, --no-beta-not\t\tPrevents construction of beta-not rete nodes for the rhs of rules. \n");
  printf("\t-s, --substitution_store\t\tTries to avoid all single mallocs for each substitution. (Enables substitution_memory.c) \n");
  printf("\nReport bugs to <hovlanddag@gmail.com>\n");
}


void print_version(){
  printf("clp (coherent logic prover) 0.10\n"); 
  printf("Copyright (C) 2011, 2012 University of Oslo and/or Dag Hovland.\n");
  printf("This is free software.  You may redistribute copies of it under the terms of\n");
  printf("the GNU General Public License <http://www.gnu.org/licenses/gpl.html>.\n");
  printf("There is NO WARRANTY, to the extent permitted by law.\n\n");
  printf("Written by Dag Hovland.\n");
}

int main(int argc, char *argv[]){
  int retval = EXIT_FAILURE;
  const struct option longargs[] = {
    {"factset", no_argument, NULL, 'f'}, 
    {"version", no_argument, NULL, 'V'}, 
    {"verbose", no_argument, NULL, 'v'}, 
    {"proof", no_argument, NULL, 'p'}, 
    {"print", required_argument, NULL, 'P'}, 
    {"help", no_argument, NULL, 'h'}, 
    {"debug", no_argument, NULL, 'g'}, 
    {"factset_lhs", no_argument, NULL, 'f'}, 
    {"x", no_argument, NULL, 'x'},
    {"max", required_argument, NULL, 'm'}, 
    {"depth-first", no_argument, NULL, 'd'}, 
    {"dry-run", no_argument, NULL, 'D'}, 
    {"tptp", no_argument, NULL, 'T'},
    {"eager", no_argument, NULL, 'e'}, 
    {"multithreaded", no_argument, NULL, 'M'}, 
    {"CL.pl", no_argument, NULL, 'C'}, 
    {"geolog", no_argument, NULL, 'G'}, 
    {"coq", no_argument, NULL, 'q'}, 
    {"factset_rhs", no_argument, NULL, 's'}, 
    {"no-beta-not", no_argument, NULL, 'n'}, 
    {"cputimer", required_argument, NULL, 't'}, 
    {"wallclocktimer", required_argument, NULL, 'w'}, 
    {"print_model", no_argument, NULL, 'o'},
    {"substitution_store", no_argument, NULL, 's'}, 
    {"all-disjuncts", no_argument, NULL, 'a'}, 
    {"single-threaded-rete", no_argument, NULL, 'S'},
    {"full-tptp", no_argument, NULL, 'F'},
    {0,0,0,0}
  };
  char shortargs[] = "w:vfVphgdoat:cCSsDP:aTGeqMnm:F";
  int longindex;
  char argval;
  verbose = false;
  proof = false;
  text = false;
  input_format = clpl_format;
  debug = false;
  print_model = false;
  output_theory = false;
  lazy = true;
  coq = false;
  use_beta_not = true;
  multithreaded = false;
  multithread_rete = true;
  factset_lhs = false;
  all_disjuncts = false;
  use_substitution_store = false;
  strat = normal_strategy;
  maxsteps = MAX_PROOF_STEPS;
  dry_run = false;
  while( ( argval = getopt_long(argc, argv, shortargs, &longargs[0], &longindex )) != -1){
    switch(argval){
      unsigned long int maxtimer;
    case 'v':
      verbose = true;
      break;
    case 'p':
      proof = true;
      break;
    case 'D':
      dry_run = true;
      break;
    case 's':
      use_substitution_store = true;
      break;
    case 'd':
      strat = clpl_strategy;
      break;
    case 'q':
      coq = true;
      break;
    case 'T':
      input_format = coherent_tptp_format;
      break;
    case 'F':
      input_format = full_tptp_format;
      break;
    case 't':
      maxtimer = get_ui_arg_opt();
      start_cpu_timer(maxtimer);
      break;
    case 'a':
      all_disjuncts = true;
      break;
    case 'S':
      multithread_rete = false;
      break;
    case 'P':
      output_theory = true;
      output_format = get_format_arg_opt();
      break;
    case 'w':
      maxtimer = get_ui_arg_opt();
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
    case 'x':
      text = true;
      break;
    case 'C':
      input_format = clpl_format;
      break;
    case 'G':
      input_format = geolog_format;
      break;
    case 'm':
      maxsteps = get_ui_arg_opt();
      break;
    default:
      fprintf(stderr, "Try '%s --help' for more information.\n", argv[0]);
      exit(EXIT_FAILURE);
    }
  } 
  if(optind < argc){
    retval = EXIT_SUCCESS;
    while( optind < argc ) {
      FILE* f;
      if(input_format == clpl_format || input_format == geolog_format || input_format == coherent_tptp_format){
	f = fopen(argv[optind], "r");
	if(f == NULL){
	  perror("main: Could not open theory file");
	  fprintf(stderr, "Filename: '%s'\n", argv[optind]); 
	  exit(EXIT_FAILURE);
	}
	if(file_prover(f, basename(argv[optind]), NULL) == EXIT_FAILURE)
	  retval = EXIT_FAILURE;
	if(fclose(f) != 0){
	  perror("main(): Could not close theory file\n");
	  fprintf(stderr, "File name: %s\n", argv[optind]); 
	  exit(EXIT_FAILURE);
	}
      } else {
	assert(input_format == full_tptp_format);
	if(file_prover(NULL, basename(argv[optind]), argv[optind]) == EXIT_FAILURE)
	  retval = EXIT_FAILURE;
      }
      optind++;
    } // end for
  } else {
    assert(optind == argc);
    printf("Reading theory from standard input\n");
    retval =  file_prover(stdin, "STDIN", "STDIN");
  }
  
  return retval;
}

