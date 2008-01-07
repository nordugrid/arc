noinst_LTLIBRARIES = libcredential.la
#noinst_PROGRAMS = test

libcredential_ladir = $(pkgincludedir)/arclib
libcredential_la_HEADERS  = Proxycertinfo.h  cert_util.h Credential.h 
libcredential_la_SOURCES  = Proxycertinfo.cpp cert_util.cpp Credential.cpp
libcredential_la_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) $(OPENSSL_CFLAGS) -I$(top_srcdir)/include
libcredential_la_LIBADD   = $(top_srcdir)/src/hed/libs/message/libarcmessage.la $(OPENSSL_LIBS)

test_SOURCES  = test.cpp
test_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
test_LDFLAGS  = $(GLIBMM_LIBS) $(EXTRA_LIBS)
test_LDADD    = libcredential.la


