#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([fontgen], [0.1], [github@jdesch.de])
AM_INIT_AUTOMAKE([1.9 foreign])
AC_CONFIG_SRCDIR([src/fontgen.c])
AC_CONFIG_HEADERS([src/config.h])

# Checks for programs.
AC_PROG_CC

# Checks for libraries.
AC_CHECK_LIB([freetype], [FT_Init_FreeType])
#AC_CHECK_FT2
AC_CHECK_FT2(, [_HAVE_FREETYPE2=1], [_HAVE_FREETYPE2=])
if test "$_HAVE_FREETYPE2"; then
  LIBS="$LIBS $FT2_LIBS"
  CFLAGS="$CFLAGS $FT2_CFLAGS"
fi

# Checks for header files.
AC_CHECK_HEADERS([stdint.h string.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_CHECK_HEADER_STDBOOL
AC_C_INLINE
AC_TYPE_UINT8_T

# Checks for library functions.
AC_FUNC_MALLOC
AC_CHECK_FUNCS([memset strrchr])

AC_CONFIG_FILES(Makefile src/Makefile)
AC_OUTPUT

