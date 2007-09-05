noinst_LTLIBRARIES=libdelegation.la
noinst_PROGRAMS=test

libdelegation_ladir=$(includedir)
libdelegation_la_HEADERS=DelegationInterface.h
libdelegation_la_SOURCES=DelegationInterface.cpp
libdelegation_la_CPPFLAGS=$(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/src/libs/ -I$(top_srcdir)/../openssl/include
libdelegation_la_LIBADD=
libdelegation_la_LDFLAGS=-L$(top_srcdir)/../openssl/lib -lssl -lcrypto

test_SOURCES=test.cpp
test_CPPFLAGS=
test_LDFLAGS=
test_LDADD=libdelegation.la
