#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.
# Not tested with autoconf < 2.59, hence this is a prerequisite

AC_PREREQ([2.59])
AC_INIT([clp], [0.2], [hovland@ifi.uio.no])
AC_COPYRIGHT([Copyright (C) 2011, 2012 University of Oslo, Norway. Author: Dag Hovland])
AM_INIT_AUTOMAKE([1.9 -Wall -Werror parallel-tests dist-bzip2])
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES])
AC_CONFIG_SRCDIR([src/prover_single.c])
AC_CONFIG_HEADERS([config.h])


# Checks for programs.
# MKDIR is not available in version 2.59, hence compatibility checked

AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_AWK
AC_PROG_CPP
m4_ifdef([AC_PROG_MKDIR_P], [AC_PROG_MKDIR_P])
AC_PROG_YACC
AM_PROG_LEX
AC_PROG_CPP

# For per-target flags
AM_PROG_CC_C_O

# Checks for pthreads functionality. 
AX_PTHREAD

# Checks for libraries.
# #FIXME: Find function in duma and Un-Comment  the next line in development versions
#AC_CHECK_LIB([duma], [free])

#FIXME: Is this line unnecessary when AM_PROG_LEX is defined above?
AC_CHECK_LIB([fl], [main])

#FIXME: What is this library?
AC_CHECK_LIB([l], [main])

#Tests for glibc math library
AC_CHECK_LIB([m], [main])

# Checks for header files.
AC_FUNC_ALLOCA
AC_CHECK_HEADERS([fcntl.h errno.h unistd.h inttypes.h libintl.h malloc.h stddef.h stdlib.h string.h math.h])

# Checks for typedefs, structures, and compiler characteristics.
# The commented macros are not available in autoconf 2.59
AC_HEADER_STDBOOL
m4_ifdef([AC_TYPE_INT16_T], [AC_TYPE_INT16_T])
m4_ifdef([AC_TYPE_INT32_T], [AC_TYPE_INT32_T])
m4_ifdef([AC_TYPE_INT8_T], [AC_TYPE_INT8_T])
AC_TYPE_SIZE_T
m4_ifdef([AC_TYPE_SSIZE_T], [AC_TYPE_SSIZE_T])
m4_ifdef([AC_TYPE_UINT16_T], [AC_TYPE_UINT16_T])
m4_ifdef([AC_TYPE_UINT32_T], [AC_TYPE_UINT32_T])
m4_ifdef([AC_TYPE_UINT8_T], [AC_TYPE_UINT8_T])

# Checks for library functions.
AC_FUNC_MALLOC
AC_FUNC_REALLOC
AC_CHECK_FUNCS([memset atexit strtoul])

#For not generating files in the maintainer's repo
AM_MAINTAINER_MODE

AC_CONFIG_FILES([Makefile
                 src/Makefile])
AC_OUTPUT
