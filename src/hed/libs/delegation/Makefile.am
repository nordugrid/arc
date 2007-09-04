noinst_LTLIBRARIES=libdelegation.la
noinst_PROGRAMS=test

libdelegation_ladir=$(includedir)
libdelegation_la_HEADERS=DelegationInterface.h
libdelegation_la_SOURCES=DelegationInterface.cpp
libdelegation_la_CPPFLAGS=$(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/src/libs/
libdelegation_la_LIBADD=
libdelegation_la_LDFLAGS=$(LIBXML2_LIBS)

test_SOURCES=test.cpp
test_CPPFLAGS=$(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/src/libs/
test_LDFLAGS=$(LIBXML2_LIBS) 
test_LDADD=libdelegation.la
