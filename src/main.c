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

/**
   The boolean lazy leads to the opposite value for propagate in the 
   alpha nodes representing atoms on the left hand side. It first sets 
   the value of net->lazy, then this is used once, in the function create_rete_axiom_node
   in axiom.c
**/
bool lazy;

extern char * optarg;
extern int optind, optopt, opterr;
bool verbose, debug, proof, dot, text, pdf, existdom, factset;
strategy strat;
unsigned long maxsteps;
typedef enum input_format_type_t {clpl_input, geolog_input} input_format_type;
input_format_type input_format;

int file_prover(FILE* f, const char* prefix){
  theory* th;
  rete_net* net;
  FILE* fp;
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
  net = create_rete_net(th, maxsteps, existdom, strat, lazy);

  if(!factset){  
    if(debug && !factset){
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
    
    init_proof_dot_writer(prefix);
  }
  steps = prover(net, factset);
  if(steps > 0){
    printf("Found a proof after %i steps that the theory has no model\n", steps);
    retval = EXIT_SUCCESS;
  } else {
    if(verbose)
      printf("Could not find proof\n");
  }
  if(!factset){
    end_proof_dot_writer(prefix);
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
  printf("\t-d, --dot\t\tGives output of proof in dot format\n");
  printf("\t-a, --pdf\t\tGives output of proof in adobe pdf format\n");
  printf("\t-t, --text\t\tGives output of proof in separate text file. Same prefix of name as input file, but with .out as suffix. \n");
  printf("\t-v, --verbose\t\tGives extra output about the proving process\n");
  printf("\t-V, --version\t\tSome info about the program, including copyright and license\n");
  printf("\t-l, --lazy\t\tUses the lazy version of RETE.\n");
  printf("\t-e, --existdom\t\tFor existential quantifiers, tries all elements in the domain before creating new constants. Not finished.\n");
  printf("\t-c, --clpl\t\tTries to emulate the strategy used by the prolog program CL.pl. The strategy is not exactly the same. \n");
  printf("\t-f, --factset\t\tUses standard fact-set based proof search in stead of the RETE alogrithm. Not finished.\n");
  printf("\t-m, --max=LIMIT\t\tMaximum number of inference steps in the proving process. 0 sets no limit\n");
  printf("\t-C, --CLpl\t\tParses the input file as in CL.pl. This is the default.\n");
  printf("\t-G, --geolog\t\tParses the input file as in Fisher's geolog.See http://johnrfisher.net/GeologUI/index.html#geolog for a description\n");
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
  const struct option longargs[] = {{"factset", no_argument, NULL, 'f'}, {"version", no_argument, NULL, 'V'}, {"verbose", no_argument, NULL, 'v'}, {"proof", no_argument, NULL, 'p'}, {"help", no_argument, NULL, 'h'}, {"debug", no_argument, NULL, 'g'}, {"dot", no_argument, NULL, 'd'}, {"pdf", no_argument, NULL, 'a'}, {"text", no_argument, NULL, 't'},{"max", required_argument, NULL, 'm'}, {"existdom", no_argument, NULL, 'e'}, {"clpl", no_argument, NULL, 'c'}, {"lazy", no_argument, NULL, 'l'}, {"CLpl", no_argument, NULL, 'C'}, {"geolog", no_argument, NULL, 'G'}, {0,0,0,0}};
  char shortargs[] = "vfVphgdaecCGlm:";
  int longindex;
  char argval;
  char * tailptr;
  verbose = false;
  proof = false;
  text = false;
  debug = false;
  dot = false;
  pdf = false;
  existdom = false;
  factset = false;
  lazy = false;
  input_format = clpl_input;
  strat = normal_strategy;
  maxsteps = MAX_PROOF_STEPS;

  while( ( argval = getopt_long(argc, argv, shortargs, &longargs[0], &longindex )) != -1){
    switch(argval){
    case 'v':
      verbose = true;
      break;
    case 'p':
      proof = true;
      break;
    case 'c':
      strat = clpl_strategy;
      break;
    case 'f':
      factset = true;
      break;
    case 'g':
      debug = true;
      break;
    case 'd':
      dot = true;
      break;
    case 'l':
      lazy = true;
      break;
    case 'h':
      print_help(argv[0]);
      exit(EXIT_SUCCESS);
      break;
    case 'V':
      print_version();
      exit(EXIT_SUCCESS);
    case 'e':
      existdom = true;
      break;
    case 't':
      text = true;
      break;
    case 'a':
      pdf = true;
      break;
    case 'C':
      input_format = clpl_input;
      break;
    case 'G':
      input_format = geolog_input;
      break;
    case 'm':
      errno = 0;
      maxsteps = strtoul(optarg, &tailptr, 0);
      if(tailptr[0] != '\0' || errno != 0){
	perror("Error with argument to option \"max\". Must be a positive integer stating maximum number of steps in the proof.\n");
	exit(EXIT_FAILURE);
      }
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

