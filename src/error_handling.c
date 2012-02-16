/* error_handling.c

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


#include "error_handling.h"
#include <errno.h>
#include <string.h>


/**
   Wrapper for function calls like fprintf, which return 
   a negative value on error
**/
void fp_err(int retval, const char* msg){
  if(retval < 0){
    perror(msg);
    assert(false);
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
    assert(false);
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


#ifdef HAVE_PTHREAD
/**
   Wrapper for pthread function calls

   Does nothing when errval is 0, otherwise prints error message
**/
void pt_err(int errval, const char* filename, int line, const char* msg){
  if(errval != 0){
    char* err_msg;
    unsigned int msg_len;
    msg_len = strlen(msg) + strlen(filename) + 20;
    err_msg = malloc(msg_len+1);
    snprintf(err_msg, msg_len, "%s (line %i): %s", filename, line, msg);
    errno = errval;
    perror(err_msg);
    free(err_msg);
    assert(false);
    exit(EXIT_FAILURE);
  }
}
#endif
