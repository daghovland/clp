/* malloc.c

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
   Functions used by several parts of the program
**/
#include "common.h"
#include <sys/time.h>
#include <sys/resource.h>
#include <string.h>
#include <errno.h>

/**
   Outputs some info about memory limits. 
   Might be useful in case of allocation failures.
**/
void show_limit_value(rlim_t lim, FILE* s){
  if(lim == RLIM_INFINITY)
    fprintf(s, "RLIM_INFINITY");
  else if (lim == RLIM_SAVED_CUR)
    fprintf(s, "RLIM_SAVED_CUR");
  else if (lim == RLIM_SAVED_MAX)
    fprintf(s, "RLIM_SAVED_MAX");
  else
    fprintf(s, "%llu", (unsigned long long) lim);
}

void show_limit(int resource, FILE* s, const char* name){
  struct rlimit limits;
  if(getrlimit(resource, &limits) != 0){
    perror("utility.c: show_limit(): cannot call getrlimit");
    assert(false);
    exit(EXIT_FAILURE);
  }
  fprintf(s, "%s limits are ", name);
  show_limit_value(limits.rlim_max, s);
  fprintf(s, " (hard) and ");
  show_limit_value(limits.rlim_cur, s);
  fprintf(s, " (soft).\n");
}

void print_mem_limits(){
  show_limit(RLIMIT_DATA, stderr, "Data segment (DATA)");
  show_limit(RLIMIT_AS, stderr, "Virtual memory (AS)");
}
 
void print_error(char* function, size_t size, int error_num){
  fprintf(stderr, "malloc.c.: %s : Memory allocation failure: %zi bytes : %s.\n", function, size, strerror(error_num));
  print_mem_limits();
}

void* malloc_tester(size_t size){
  void* ret_val = malloc(size);
  if(ret_val == NULL){
    int error_num = errno;
    print_error("malloc", size, error_num);
    assert(false);
    exit(EXIT_FAILURE);    
  }
  return ret_val;
}

void* calloc_tester(size_t nmemb, size_t size){
  void* ret_val = calloc(nmemb, size);
  if(ret_val == NULL){
    int error_num = errno;
    print_error("calloc", size * nmemb , error_num);
    assert(false);
    exit(EXIT_FAILURE);    
  }
  return ret_val;
}


void* realloc_tester(void* ptr, size_t size){
  void* ret_val = realloc(ptr, size);
  if(ret_val == NULL){
    int error_num = errno;
    print_error("realloc", size, error_num);
    assert(false);
    exit(EXIT_FAILURE);    
  }
  return ret_val;
}
