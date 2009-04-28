# The AC_INIT macro can take any source file as an argument. It just checks that 
# the file is there, which should, in turn, mean that the source directory is there.
AC_INIT(src/echo.cpp)


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
#AC_CONFIG_FILES


# The AM_INIT_AUTOMAKE line adds several standard checks. It takes the program
# name and version number as arguments.
AM_INIT_AUTOMAKE(echoservice,0.1)


# Indicates the name of the Makefile which will be generated.
# MY_MODULES="src"
# MY_MAKEFILES="`for D in $MY_MODULES; do echo $ac_n ' '$D/Makefile; done`"
# AC_OUTPUT(
# Makefile,
#  echo $MY_MAKEFILES
#  )
#  AC_CONFIG_FILES([$MY_MAKEFILES])
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
#AC_OUTPUT(Makefile src/Makefile src/echoservice/Makefile src/timeservice/Makefile src/clients/Makefile src/sec_echoservice/Makefile)



# Loads Libtool for creating libraries
AC_PROG_LIBTOOL
#LT_INIT([disable-shared]) # makes the build process faster!!!! Good for debugging
#LT_INIT([shared])
#LT_PREREQ([2.2])
#LT_INIT([dlopen])

# check libxml
#PKG_CHECK_MODULES(LIBXML2, [libxml-2.0 >= 2.4.0])
#AC_SUBST(LIBXML2_CFLAGS)
#AC_SUBST(LIBXML2_LIBS)

# check glibmm
#PKG_CHECK_MODULES(GLIBMM, [glibmm-2.4 >= 2.4.7])
#AC_SUBST(GLIBMM_CFLAGS)
#AC_SUBST(GLIBMM_LIBS)



# This will allow you to build code in sub-directories into temporary libraries,
# which make will then link in with the rest of the code.
# AC_RANLIB
