/* error_handling.h

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

#ifndef __INCLUDED_ERROR_HANDLING_H
#define __INCLUDED_ERROR_HANDLING_H

#include "common.h"
void fp_err(int retval, const char* msg);
void sys_err(int retval, const char* msg);
FILE* file_err(FILE* retval, const char* msg);
#ifdef HAVE_PTHREAD
void pt_err(int , const char*, int, const char* );
#endif


#endif
