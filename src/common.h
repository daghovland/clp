/* common.h

   Copyright 2008 Free Software Foundation

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

/* This is included by all source files in the project */

#define _GNU_SOURCE

/**
   The name used in the coq proof writer for the set domain
**/
#define DOMAIN_SET_NAME "domain_set"
/**
   Debugs the parsers
   
   Remember to also uncomment yydebug = 1 in the parser function
**/
#define YYDEBUG 1
#include <stdio.h>

/**
   Set the maximum number of steps in a proof
**/
#define   MAX_PROOF_STEPS 300

/**
   Uncomment the line below to test the fresh_constants.c without gcc extensions __sync_fetch ...
**/
//#undef __GNUC__

/**
  If defined, the lazy variant of the RETE algorithm is used. 
  As of Aug 9, 2011, this is not finished.
**/
#define LAZY_RETE
/**
  Uncomment to prevent threading functionality from being built in
**/
//#undef HAVE_PTHREAD

/**
   Uncomment this line to get assertions, which are used for a sort of 
   run-time unit testing. You can also change this in Makefile.am, if you 
   have autotools installed
**/
//#define NDEBUG

/**
   To get loads of debugging output about the rete state, uncomment this
**/
//#define  __DEBUG_RETE_STATE 

/**
   To get dot output of state of rete net at each proof step
**/
//#define RETE_STATE_DEBUG_DOT
//#define RETE_STATE_DEBUG_TXT
/**
   For debugging the parallel execution involving RETE, uncomment this
**/
//#define __DEBUG_RETE_PTHREAD

#ifndef __INCLUDED_COMMON_H
#define __INCLUDED_COMMON_H

#if HAVE_CONFIG_H
# include <config.h>
#endif



#include "malloc.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>

#endif
