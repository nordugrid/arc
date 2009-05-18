# REMEMBER THAT EACH MAKEFILE.AM HAS TO BE ADDED TO THE LIST
# IN THE AC_OUTPUT MACRO IN THE TOPLEVEL configure.ac

EXTRA_DIST = clientHED.xml clientCert.pem clientKey.pem serviceCA.pem

LIBTOOL = libtool

AM_CPPFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) $(ARC_CFLAGS) $(all_includes)

# BUILDING EXECUTABLES
bin_PROGRAMS = secechoclient

secechoclient_SOURCES =  secechoclient.cpp
secechoclient_LDFLAGS = $(ARC_LIBS) -lmccsoap
secechoclient_CPPFLAGS = $(AM_CPPFLAGS)                    	# -I@top_srcdir@/src/includes






