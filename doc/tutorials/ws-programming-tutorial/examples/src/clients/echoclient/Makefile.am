# REMEMBER THAT EACH MAKEFILE.AM HAS TO BE ADDED TO THE LIST
# IN THE AC_OUTPUT MACRO IN THE TOPLEVEL configure.ac

EXTRA_DIST = 

LIBTOOL = libtool

AM_CPPFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) $(ARC_CFLAGS) $(all_includes)

# BUILDING EXECUTABLES
bin_PROGRAMS = echoclient
echoclient_SOURCES = echoclient.cpp
echoclient_LDFLAGS = $(ARC_LIBS) -lmccsoap
echoclient_CPPFLAGS = $(AM_CPPFLAGS) 				# -I@top_srcdir@/src/includes







