# REMEMBER THAT EACH MAKEFILE.AM HAS TO BE ADDED TO THE LIST
# IN THE AC_OUTPUT MACRO IN THE TOPLEVEL configure.ac

EXTRA_DIST = clientHED.xml clientCert.pem clientKey.pem serviceCA.pem

LIBTOOL = libtool

AM_CPPFLAGS = -I/usr/include/glibmm-2.4/ -I/usr/include/glib-2.0 -I/usr/include/libxml2/ -I/usr/lib/glib-2.0/include/ -I/usr/include/sigc++-2.0 -I/usr/lib/sigc++-2.0/include -I/usr/lib/glibmm-2.4/include/ -I/usr/lib/arc/ $(all_includes)    # HAVE TO HAVE $(all_includes)

# BUILDING EXECUTABLES
bin_PROGRAMS = tlsechoclient

tlsechoclient_SOURCES =  tlsechoclient.cpp
tlsechoclient_LDFLAGS = -L/usr/lib/arc -lmccsoap                   #
tlsechoclient_CPPFLAGS = $(AM_CPPFLAGS)                    	# -I@top_srcdir@/src/includes






