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
#include <malloc.h>
#include "common.h"

void* malloc_tester(size_t size){
  void* ret_val = malloc(size);
  if(ret_val == NULL){
    fprintf(stderr, "Memory allocation failure: malloc(%i)\n", (int) size);
    perror("malloc returned null\n");
    exit(EXIT_FAILURE);
  }
  return ret_val;
}

void* calloc_tester(size_t nmemb, size_t size){
  void* ret_val = calloc(nmemb, size);
  if(ret_val == NULL){
    fprintf(stderr, "Memory allocation failure: calloc(%i, %i)\n", (int) nmemb, (int) size);
    perror("calloc returned null\n");
    exit(EXIT_FAILURE);
  }
  return ret_val;
}


void* realloc_tester(void* ptr, size_t size){
  void* ret_val = realloc(ptr, size);
  if(ret_val == NULL){
    fprintf(stderr, "Memory allocation failure: realloc(ptr, %i)\n", (int) size);
    perror("realloc returned null\n");
    exit(EXIT_FAILURE);
  }
  return ret_val;
}
