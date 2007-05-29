lib_LTLIBRARIES= libmcctls.la
noinst_PROGRAMS=test
SOURCES = PayloadTLSStream.cpp PayloadTLSSocket.cpp MCCTLS.cpp

libmcctls_ladir=$(includedir)
libmcctls_la_HEADERS=PayloadTLSStream.h PayloadTLSSocket.h MCCTLS.h
libmcctls_la_SOURCES=PayloadTLSStream.cpp PayloadTLSSocket.cpp MCCTLS.cpp
libmcctls_la_CPPFLAGS=$(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) $(OPENSSL_CFLAGS) -I$(top_srcdir)/src/libs/
libmcctls_la_LIBADD=$(top_srcdir)/src/hed/libs/message/libmessage.la
libmcctls_la_LDFLAGS=$(LIBXML2_LIBS) $(OPENSSL_LIBS) -lpthread

test_SOURCES=$(SOURCES) test.cpp
test_CPPFLAGS=$(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) $(OPENSSL_CFLAGS) -I$(top_srcdir)/src/libs/
test_LDFLAGS=$(LIBXML2_LIBS) $(OPENSSL_LIBS) -lpthread
test_LDADD=$(top_srcdir)/src/libs/common/libarccommon.la $(top_srcdir)/src/hed/libs/message/libmessage.la
