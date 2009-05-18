# REMEMBER THAT EACH MAKEFILE.AM HAS TO BE ADDED TO THE LIST
# IN THE AC_OUTPUT MACRO IN THE TOPLEVEL configure.ac

EXTRA_DIST = 

LIBTOOL = libtool

AM_CPPFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) $(ARC_CFLAGS) $(all_includes)

# BUILDING EXECUTABLES
bin_PROGRAMS = timeclient

timeclient_SOURCES = timeclient.cpp
timeclient_LDFLAGS = $(ARC_LIBS) -lmccsoap 
timeclient_CPPFLAGS = $(AM_CPPFLAGS) 				# -I@top_srcdir@/src/includes
# timeclient_LDADD						# LIST OF LIBS TO BE CONNECTED WITH THE LIB.






