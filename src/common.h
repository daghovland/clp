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

#ifndef __INCLUDED_COMMON_H
#define __INCLUDED_COMMON_H


/* This is included by all source files in the project */

#define _GNU_SOURCE

#if HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>

/**
   If defined (not commented) then the original implementation of timestamps is used.
   Otherwise, the new, linked list, implementation is used
**/
//#define USE_TIMESTAMP_ARRAY
/**
   If defined, the array implemetation of memory for timestamps is used. 
   Otherwise, malloc is used for each timestamp link. 
   Presently (14 feb 2012) the malloc version has a bug, so should not be used. 
   I believe it is only interesting for debugging, so should perhaps be removed
**/
#define USE_TIMESTAMP_STORE_ARRAY

/**
   The name used in the coq proof writer for the set domain
**/
#define DOMAIN_SET_NAME "domain_set"

/**
   The name used when inserting variables only occurring in equalities
**/
#define DOMAIN_PRED_NAME "dom"
/**
   Debugs the parsers
   
   Remember to also uncomment yydebug = 1 in the parser function
**/
//#define YYDEBUG 1

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
//#define DEBUG_RETE_INSERT
/**
   To get dot output of state of rete net at each proof step
**/
//#define RETE_STATE_DEBUG_DOT
//#define RETE_STATE_DEBUG_TXT
/**
   For debugging the parallel execution involving RETE, uncomment this
**/
//#define __DEBUG_RETE_PTHREAD


/**
   These are gcc builtins to help avoid buffer overflow, or at least
   give better error messages
**/
#ifdef __GNUC__
#undef memcpy
#define bos0(dest) __builtin_object_size (dest, 0)
#define memcpy(dest, src, n)				\
  __builtin___memcpy_chk (dest, src, n, bos0 (dest))
#undef strcpy
#define strcpy(dest, src)				\
  __builtin___strcpy_chk (dest, src, bos0 (dest))
#undef strncpy
#define strncpy(dest, src, n)				\
  __builtin___strncpy_chk (dest, src, n, bos0 (dest))
#undef sprintf
#define sprintf(str, fmt, vars...)			\
  __builtin___sprintf_chk (str, 0, bos0(str), fmt, vars )
#undef snprintf
#define snprintf(str, n, fmt, vars...)				\
  __builtin___snprintf_chk (str, n, 0, bos0(str), fmt, vars )
#endif

#include "malloc.h"

#endif
