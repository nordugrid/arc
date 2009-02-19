# REMEMBER THAT EACH MAKEFILE.AM HAS TO BE ADDED TO THE LIST
# IN THE AC_OUTPUT MACRO IN THE TOPLEVEL configure.ac

EXTRA_DIST = arched_sec_echoservice.xml clientCA.pem clientlist.txt policy.xml serviceCert.pem serviceKey.pem

LIBTOOL = libtool

AM_CPPFLAGS = -I/usr/include/glibmm-2.4/ -I/usr/include/glib-2.0 -I/usr/include/libxml2/ -I/usr/lib/glib-2.0/include/ -I/usr/include/sigc++-2.0 -I/usr/lib/sigc++-2.0/include -I/usr/lib/glibmm-2.4/include/ $(all_includes)    # HAVE TO HAVE $(all_includes)

# BUILDING EXECUTABLES
#bin_PROGRAMS = secechoservice
#secechoservice_SOURCES = sec_echoservice.h sec_echoservice.cpp
#secechoservice_LDFLAGS = -L/usr/lib/arc -lmcchttp 
#secechoservice_CPPFLAGS = $(AM_CPPFLAGS) 			# -I@top_srcdir@/src/includes
# secechoservice_LDADD					# LIST OF LIBS TO BE CONNECTED WITH THE LIB.

# BUILDING LIBRARIES
lib_LTLIBRARIES = libsecechoservice.la
libsecechoservice_la_SOURCES = secechoservice.h secechoservice.cpp
libsecechoservice_la_LIBADD =					# libbla.la 
libsecechoservice_la_CXXFLAGS =					# Includes
libsecechoservice_la_LDFLAGS = -L/usr/lib/arc -lmcchttp -avoid-version -module # -no-undefined



# Use a LDADD_ macro in the Makefile.am of a higher directory to link the temporary 
# library with any code that uses it. For example,
#LDADD = foofiles/libfoo.a

#noinst_LIBRARIES = libfoo.a

# g++  -g -O2   -o echoervice echo.o -L /usr/lib/arc -lmcchttp

# lib_LTLIBRARIES = something-1.0.la                    # CALLS libtool
# something_la_SOURCES = something.h something.cc       # SOURCE FILES
# something_la_LIBADD = sub/libsubstuff.la              # LIST OF LIBS TO BE CONNECTED WITH THE LIB. no recursion!!!!!!!
# something_la_LDFLAGS = $(all_libraries) -no-undefined # $(all_libraries) contaons the -L Flags needed for QT and KDE.

