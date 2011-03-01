#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.65])
AC_INIT([prover], [0.1], [hovland@ifi.uio.no])
AC_COPYRIGHT([Copyright (C) 2011 University of Oslo, Norway. Author: Dag Hovland])
AM_INIT_AUTOMAKE([parallel-tests -Wall  -Werror dist-bzip2])
AC_CONFIG_SRCDIR([src/prover.c])
AC_CONFIG_HEADERS([config.h])

# Checks for programs.
AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_AWK
AC_PROG_CPP
AC_PROG_MKDIR_P
AC_PROG_YACC
AM_PROG_LEX
AC_PROG_CPP

# Checks for pthreads functionality
AX_PTHREAD

# Checks for libraries.
# #FIXME: Find function in duma and Un-Comment  the next line in development versions
#AC_CHECK_LIB([duma], [free])

#FIXME: Is this line unnecessary when AM_PROG_LEX is defined above?
AC_CHECK_LIB([fl], [main])

#FIXME: What is this library?
AC_CHECK_LIB([l], [main])

# Checks for header files.
AC_FUNC_ALLOCA
AC_CHECK_HEADERS([fcntl.h errno.h unistd.h inttypes.h libintl.h malloc.h stddef.h stdlib.h string.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_TYPE_INT16_T
AC_TYPE_INT32_T
AC_TYPE_INT8_T
AC_TYPE_SIZE_T
AC_TYPE_SSIZE_T
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT8_T

# Checks for library functions.
AC_FUNC_MALLOC
AC_FUNC_REALLOC
AC_CHECK_FUNCS([memset atexit strtoul])



AC_CONFIG_FILES([Makefile
                 src/Makefile])
AC_OUTPUT
