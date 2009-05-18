# REMEMBER THAT EACH MAKEFILE.AM HAS TO BE ADDED TO THE LIST
# IN THE AC_OUTPUT MACRO IN THE TOPLEVEL configure.ac

EXTRA_DIST = arched_timeservice.xml

LIBTOOL = libtool

AM_CPPFLAGS =  $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) $(ARC_CFLAGS) $(all_includes)




# BUILDING EXECUTABLES
#bin_PROGRAMS = timeservice
#timeservice_SOURCES = timeservice.h timeservice.cpp
#timeservice_LDFLAGS = -L/usr/lib/arc -lmcchttp 
#timeservice_CPPFLAGS = $(AM_CPPFLAGS) 			# -I@top_srcdir@/src/includes
# timeservice_LDADD					# LIST OF LIBS TO BE CONNECTED WITH THE LIB.


# BUILDING LIBRARIES
lib_LTLIBRARIES = libtimeservice.la
libtimeservice_la_SOURCES = timeservice.h timeservice.cpp
libtimeservice_la_LIBADD =					# libbla.la 
libtimeservice_la_CXXFLAGS =					# Includes
libtimeservice_la_LDFLAGS = $(ARC_LIBS) -lmccsoap  -avoid-version -module # -no-undefined



# Use a LDADD_ macro in the Makefile.am of a higher directory to link the temporary 
# library with any code that uses it. For example,
#LDADD = foofiles/libfoo.a

#noinst_LIBRARIES = libfoo.a

# g++  -g -O2   -o timeervice echo.o -L /usr/lib/arc -lmcchttp

# lib_LTLIBRARIES = something-1.0.la                    # CALLS libtool
# something_la_SOURCES = something.h something.cc       # SOURCE FILES
# something_la_LIBADD = sub/libsubstuff.la              # LIST OF LIBS TO BE CONNECTED WITH THE LIB. no recursion!!!!!!!
# something_la_LDFLAGS = $(all_libraries) -no-undefined # $(all_libraries) contaons the -L Flags needed for QT and KDE.

