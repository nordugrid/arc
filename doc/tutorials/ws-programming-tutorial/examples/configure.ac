# The AC_INIT macro can take any source file as an argument. It just checks that 
# the file is there, which should, in turn, mean that the source directory is there.
AC_INIT(src/Makefile.am)


# Indicates that the source code may be in C. If the source code is C++ then
# we also need 
AC_PROG_CC
AC_PROG_CXX


# Will generate an install target so that users may just type 'make install' 
# to install the software.
AC_PROG_INSTALL


# The AM_CONFIG_HEADER(config.h) line indicates that you will be using a config.h file. 
# autoconf will then need a config.h.in file, which it processes to create the config.h
# file. This is #included by your source code and provides a way for people to customise 
# the configuration for their platform, via #defines. config.h.in can be generated 
# automatically with the autoheader tool.
AC_CONFIG_SRCDIR([config.h.in])
AC_CONFIG_HEADER(config.h)


# The AM_INIT_AUTOMAKE line adds several standard checks. It takes the program
# name and version number as arguments.
AM_INIT_AUTOMAKE(echoservice,0.1)

# check glibmm
PKG_CHECK_MODULES(GLIBMM, [glibmm-2.4 >= 2.4.7])
AC_SUBST(GLIBMM_CFLAGS)
AC_SUBST(GLIBMM_LIBS)

# check libxml
PKG_CHECK_MODULES(LIBXML2, [libxml-2.0 >= 2.4.0])
AC_SUBST(LIBXML2_CFLAGS)
AC_SUBST(LIBXML2_LIBS)


# ADD A TEST TO DETERMINE ARC INSTALLATION PATH (or if it is even installed)
AC_MSG_CHECKING([location of ARC])

ARC_EXEC="arched"
ARC_CFLAGS=""
ARC_LIBS=""
ARC_INSTALLED=no

ARCHED_DIR=`which $ARC_EXEC`
if test "x$ARCHED_DIR" != "x" ; then
  ARC_INSTALL_DIR=$(dirname $(dirname $(ldd $ARCHED_DIR | grep libarccommon | cut -f2 -d\> | cut -f2 -d\  )))

  if test "x$ARC_INSTALL_DIR" != "x" ; then

    ARC_CFLAGS=-I$ARC_INSTALL_DIR/include
    ARC_LIBS="-L$ARC_INSTALL_DIR/lib -L$ARC_INSTALL_DIR/lib/arc"
    ARC_INSTALLED=yes
    AC_MSG_RESULT([found])
  else
     AC_MSG_RESULT([not found])
  fi
else
   AC_MSG_RESULT([not found])
fi

AC_MSG_WARN($ARC_CFLAGS)
AC_MSG_WARN($ARC_LIBS)
AC_MSG_WARN($ARC_INSTALL_DIR)

AC_SUBST(ARC_CFLAGS)
AC_SUBST(ARC_LIBS)

AC_OUTPUT


AC_CONFIG_FILES([
		Makefile  
		src/Makefile  
		src/clients/Makefile
		src/clients/timeclient/Makefile
		src/clients/echoclient/Makefile
		src/clients/secechoclient/Makefile
		src/clients/tlsechoclient/Makefile
		src/services/Makefile
		src/services/timeservice/Makefile
		src/services/echoservice/Makefile
		src/services/secechoservice/Makefile
		src/services/tlsechoservice/Makefile
 ])
AC_OUTPUT



# Loads Libtool for creating libraries
AC_PROG_LIBTOOL
