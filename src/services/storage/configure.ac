#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.56)
AC_CANNONICAL_SYSTEM
AC_INIT([nordugrid-arc-chelonia],[0.1],[http://bugzilla.nordugrid.org/])
AM_INIT_AUTOMAKE([foreign])
AC_CONFIG_SRCDIR([Makefile.am])

AC_CHECK_PROGS(PYTHON,python)
if test "X$PYTHON" != "X" ; then
    changequote(<<, >>)dnl
    PYTHON_SITE_LIB=`$PYTHON -c 'from distutils import sysconfig; print sysconfig.get_python_lib(0,0,"${prefix}")'`
    changequote([, ])dnl
    AC_SUBST(PYTHON_SITE_LIB)
else
    AC_MSG_ERROR([python binary not found])
fi

AC_CONFIG_FILES([Makefile
                 nordugrid-arc-chelonia.spec
                 storage_service.xml.example
                 chelonia.1
                 librarian/Makefile 
                 librarian/schema/Makefile 
                 cli/Makefile 
                 shepherd/Makefile 
                 shepherd/schema/Makefile 
                 ahash/Makefile 
                 ahash/schema/Makefile 
                 bartender/Makefile
                 bartender/schema/Makefile
                 bartender/gateway/Makefile 
])
AC_OUTPUT
