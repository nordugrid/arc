DIST_SUBDIRS = test
SUBDIRS = $(TEST_DIR)

noinst_LTLIBRARIES = libarcdelegation.la
#noinst_PROGRAMS = test

libarcdelegation_ladir = $(pkgincludedir)/delegation
libarcdelegation_la_HEADERS  = DelegationInterface.h
libarcdelegation_la_SOURCES  = DelegationInterface.cpp
libarcdelegation_la_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) $(OPENSSL_CFLAGS) -I$(top_srcdir)/include
libarcdelegation_la_LIBADD   = $(top_srcdir)/src/hed/libs/message/libarcmessage.la $(OPENSSL_LIBS)

#test_SOURCES  = test.cpp
#test_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
#test_LDFLAGS  = $(GLIBMM_LIBS) $(EXTRA_LIBS)
#test_LDADD    = libarcdelegation.la $(top_srcdir)/src/hed/libs/message/libarcmessage.la $(OPENSSL_LIBS)
