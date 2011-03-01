/* fresh_constants.c

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

#include "fresh_constants.h"
#include "common.h"

#ifndef __GNUC__

#include <pthread.h>
#include <errno.h>
pthread_mutex_t fresh_const_counter_mutex = PTHREAD_MUTEX_INITIALIZER;

/**
   Wrapper for testing error return value of pthread functions. 
   They consistently use the return value, and not errno.
   Descriptions are taken from the man page for pthread_mutex_lock
**/
void error_test_pthread(int errval){
  if(errval != 0){
    const char* name = "?";
    const char* desc = "";
    switch(errval){
    case EBUSY:
      name = "EBUSY";
      desc = "The mutex could not be acquired because it was already locked.";
      break;
    case EINVAL:
      name = "EINVAL";
      desc = "The value specified by mutex does not refer to an initialized mutex object.";
      break;
    case EAGAIN:
      name = "EAGAIN";
      desc = "The mutex could not be acquired because the maximum number of recursive locks for mutex has been exceeded.";
      break;
    case EDEADLK:
      name = "EDEADLK";
      desc = "The current thread already owns the mutex.";
      break;
    case EPERM:
      name = "EPERM";
      desc = "The current thread does not own the mutex.";
      break;
    }
    fprintf(stderr, "pthread function returned non-0 value %i (Name: , Description: ). Aborting\n", errval, name, desc);
    exit(EXIT_FAILURE);
  }
}

#endif

fresh_const_counter init_fresh_const(size_t num_vars){
  return calloc_tester(num_vars, sizeof(unsigned int));
}

/**
   This functions is thread-safe
   
   It uses a gcc built-in function, or pthread mutexes
**/
unsigned int next_fresh_const_no(fresh_const_counter fresh, size_t var_no){
  unsigned int retval;
#ifdef  __GNUC__
  retval = __sync_fetch_and_add (& (fresh[var_no]), 1);
#else
  error_test_pthread( pthread_mutex_lock(&fresh_const_counter_mutex) );

  retval = fresh[var_no]++;
  error_test_pthread( pthread_mutex_unlock(&fresh_const_counter_mutex) );
#endif
  return retval;
}

