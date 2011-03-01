/* parser_helper.c

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

/**
   Functions used by the geolog_parser to create the rules data structures 

**/
#include "common.h"
#include "geolog.h"




theory create_theory(axiom axm){
  theory ret_val;
  ret_val.size_axioms = 100;
  ret_val.axioms = calloc(ret_val.size_axioms, sizeof(axiom));
  ret_val.axioms[0] = axm;
  ret_val.n_axioms = 1;
  return ret_val;
}

theory extend_theory(theory th, axiom ax){
  th.n_axioms++;
  if(th.n_axioms >= th.size_axioms){
    th.size_axioms *= 2;
    th.axioms = realloc(th.axioms, th.size_axioms * sizeof(axiom));
  }
  th.axioms[th.n_axioms-1] = ax;
  return th;
}


conjunction create_conjunction(atom at){
  conjunction ret_val;
  ret_val.size_args = 100;
  ret_val.args = calloc(ret_val.size_args, sizeof(atom));
  ret_val.args[0] = at;
  ret_val.n_args = 1;
  return ret_val;
}

conjunction extend_conjunction(conjunction conj, atom at){
  conj.n_args++;
  if(conj.n_args >= conj.size_args){
    conj.size_args *= 2;
    conj.args = realloc(conj.args, conj.size_args * sizeof(atom));
  }
  conj.args[conj.n_args-1] = at;
  return conj;
}


disjunction create_disjunction(conjunction disj){
  disjunction ret_val;
  ret_val.size_args = 100;
  ret_val.args = calloc(ret_val.size_args, sizeof(conjunction));
  ret_val.args[0] = disj;
  ret_val.n_args = 1;
  return ret_val;
}

disjunction extend_disjunction(disjunction disj, conjunction conj){
  disj.n_args++;
  if(disj.n_args >= disj.size_args){
    disj.size_args *= 2;
    disj.args = realloc(disj.args, disj.size_args * sizeof(conjunction));
  }
  disj.args[disj.n_args-1] = conj;
  return disj;
}

atom create_atom(char* name, term_list args){
  atom ret_val;
  ret_val.predicate = name;
  ret_val.args.args = args.args;
  ret_val.args.n_args = args.n_args;
  ret_val.args.size_args = args.size_args;
  return ret_val;
}
atom create_prop_variable(char* name){
  atom ret_val;
  ret_val.predicate = name;
  ret_val.args.n_args = 0;
  ret_val.args.size_args = 0;
  return ret_val;
}
  

term create_term(char* name, term_list args){
    term ret_val;
    ret_val.name = name; 
    ret_val.args.args = args.args;
    ret_val.args.n_args = args.n_args;
    ret_val.args.size_args = args.size_args;
    return ret_val;
}
term create_constant(char* name){
  term ret_val;
  ret_val.name = name;
  ret_val.args.n_args = 0;
  return ret_val;
}

term_list create_term_list(term t){
  term_list ret_val;
  ret_val.n_args = 1;
  ret_val.size_args = 10;
  ret_val.args = malloc(ret_val.size_args * sizeof(term));
  ret_val.args[0] = t;
  return ret_val;
}
term_list extend_term_list(term_list tl, term t){
  tl.n_args ++;
  if(tl.n_args >= tl.size_args){
    tl.size_args *= 2;
    tl.args = realloc(tl.args, tl.size_args * sizeof(term) );
  }
  tl.args[tl.n_args-1] = t;
  return tl;
}

