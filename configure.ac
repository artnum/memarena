AC_INIT([libmemarena], [0.1], [etienne@artnum.ch])
AC_CONFIG_SRCDIR([src/memarena.c])
AC_CONFIG_HEADERS([config.h])
AM_INIT_AUTOMAKE([foreign subdir-objects -Wall])
AC_PROG_CC
AC_PROG_INSTALL

LT_INIT([dlopen])

AC_CHECK_HEADERS([fcntl.h sys/mman.h unistd.h], [], [
  AC_MSG_ERROR([required header not found])
])
AC_CHECK_FUNCS([mmap munmap getpagesize], [], [
  AC_MSG_ERROR([required function missing: $ac_func])
])

AC_SEARCH_LIBS([mmap], [rt], [], [])
AC_SEARCH_LIBS([getpagesize], [rt], [], [])

AC_CONFIG_FILES([Makefile memarena.pc])
AC_OUTPUT
