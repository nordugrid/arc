noinst_LTLIBRARIES=libdelegation.la
noinst_PROGRAMS=test

libdelegation_ladir=$(includedir)
libdelegation_la_HEADERS=DelegationInterface.h
libdelegation_la_SOURCES=DelegationInterface.cpp
libdelegation_la_CXXFLAGS=$(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) $(OPENSSL_CFLAGS) -I$(top_srcdir)/include
libdelegation_la_LIBADD=$(top_srcdir)/src/hed/libs/message/libmessage.la
libdelegation_la_LDFLAGS=$(OPENSSL_LIBS)

test_SOURCES=test.cpp
test_CXXFLAGS=$(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
test_LDFLAGS=$(GLIBMM_LIBS)
test_LDADD=libdelegation.la
