#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.57)
AC_CANNONICAL_SYSTEM
AC_INIT([arc],[1.0],[knowarc-support@nordugrid.org])
AM_INIT_AUTOMAKE
AC_CONFIG_SRCDIR([Makefile.am])
AM_CONFIG_HEADER([config.h])

# Checks for programs.
AC_PROG_CXX
AC_PROG_CC_STDC
AC_PROG_AWK
AC_PROG_INSTALL
AC_PROG_LN_S

AM_PROG_LIBTOOL

AC_PATH_PROG(PKG_CONFIG, pkg-config, no)
if test "xPKG_CONFIG" = "xno"; then
        AC_MSG_ERROR([ *** pkg-config not found])
else
        pkgconfigdir=${libdir}/pkgconfig
        AC_SUBST(pkgconfigdir)
fi

# using pkgconfig
# check glibmm
PKG_CHECK_MODULES(GLIBMM, [glibmm-2.4 >= 2.6.0])
AC_SUBST(GLIBMM_CFLAGS)
AC_SUBST(GLIBMM_LIBS)
PKG_CHECK_MODULES(LIBXML2, [libxml-2.0 >= 2.6.0])
AC_SUBST(LIBXML2_CFLAGS)
AC_SUBST(LIBXML2_LIBS)

# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([fcntl.h netdb.h stdlib.h sys/socket.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_C_CONST
AC_TYPE_OFF_T

# Checks for library functions.
AC_FUNC_MALLOC
AC_FUNC_REALLOC
AC_CHECK_FUNCS([memmove memset socket strcasecmp strchr strncasecmp strtol])

AC_CONFIG_FILES([Makefile \
                 src/Makefile \
                 src/hed/Makefile \
                 src/hed/wsrf/Makefile \
                 src/hed/ws-addressing/Makefile \
                 src/hed/libs/Makefile \
                 src/hed/libs/loader/Makefile \
                 src/libs/Makefile \
                 src/libs/common/Makefile \
		 doc/Makefile
])

AC_OUTPUT
