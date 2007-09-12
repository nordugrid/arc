pkglib_LTLIBRARIES = libmcctls.la
noinst_PROGRAMS=test

libmcctls_la_SOURCES=PayloadTLSStream.cpp PayloadTLSSocket.cpp MCCTLS.cpp BIOMCC.cpp PayloadTLSMCC.cpp \
                     PayloadTLSStream.h   PayloadTLSSocket.h   MCCTLS.h   BIOMCC.h   PayloadTLSMCC.h
libmcctls_la_CXXFLAGS=$(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) $(OPENSSL_CFLAGS) -I$(top_srcdir)/include
libmcctls_la_LIBADD=$(top_srcdir)/src/hed/libs/common/libarccommon.la $(top_srcdir)/src/hed/libs/message/libmessage.la
libmcctls_la_LDFLAGS=$(LIBXML2_LIBS) $(OPENSSL_LIBS) -lpthread -no-undefined -avoid-version -module

test_SOURCES=test.cpp
test_CXXFLAGS=$(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) $(OPENSSL_CFLAGS) -I$(top_srcdir)/include
test_LDFLAGS=$(LIBXML2_LIBS) $(OPENSSL_LIBS) -lpthread
test_LDADD=libmcctls.la $(top_srcdir)/src/hed/libs/common/libarccommon.la $(top_srcdir)/src/hed/libs/message/libmessage.la
