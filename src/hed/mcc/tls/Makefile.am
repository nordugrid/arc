pkglib_LTLIBRARIES = libmcctls.la
noinst_PROGRAMS=test

libmcctls_ladir=$(includedir)
libmcctls_la_HEADERS=PayloadTLSStream.h   PayloadTLSSocket.h   MCCTLS.h   BIOMCC.h   PayloadTLSMCC.h
libmcctls_la_SOURCES=PayloadTLSStream.cpp PayloadTLSSocket.cpp MCCTLS.cpp BIOMCC.cpp PayloadTLSMCC.cpp
libmcctls_la_CXXFLAGS=$(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) $(OPENSSL_CFLAGS) -I$(top_srcdir)/src/libs -I$(top_srcdir)/src/hed/libs
libmcctls_la_LIBADD=$(top_srcdir)/src/hed/libs/message/libmessage.la
libmcctls_la_LDFLAGS=$(LIBXML2_LIBS) $(OPENSSL_LIBS) -lpthread -no-undefined -avoid-version -module

test_SOURCES=test.cpp
test_CXXFLAGS=$(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) $(OPENSSL_CFLAGS) -I$(top_srcdir)/src/libs -I$(top_srcdir)/src/hed/libs
test_LDFLAGS=$(LIBXML2_LIBS) $(OPENSSL_LIBS) -lpthread
test_LDADD=libmcctls.la $(top_srcdir)/src/libs/common/libarccommon.la $(top_srcdir)/src/hed/libs/message/libmessage.la
