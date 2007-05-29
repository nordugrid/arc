lib_LTLIBRARIES= libmcchttp.la
noinst_PROGRAMS=test http_test http_test_withtls
SOURCES = MCCHTTP.cpp PayloadHTTP.cpp

libmcchttp_ladir=$(includedir)
libmcchttp_la_HEADERS=MCCHTTP.h PayloadHTTP.h
libmcchttp_la_SOURCES=MCCHTTP.cpp PayloadHTTP.cpp
libmcchttp_la_CPPFLAGS=$(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/src/libs/
libmcchttp_la_LIBADD=$(top_srcdir)/src/hed/libs/message/libmessage.la
libmcchttp_la_LDFLAGS=$(LIBXML2_LIBS)

test_SOURCES=$(SOURCES) test.cpp
test_CPPFLAGS=$(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/src/libs/
test_LDFLAGS=$(LIBXML2_LIBS)
test_LDADD=$(top_srcdir)/src/libs/common/libarccommon.la  $(top_srcdir)/src/hed/libs/message/libmessage.la

http_test_SOURCES=PayloadHTTP.cpp ../tcp/PayloadTCPSocket.cpp  http_test.cpp
http_test_CPPFLAGS=$(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/src/libs/
http_test_LDFLAGS=$(LIBXML2_LIBS)
http_test_LDADD=$(top_srcdir)/src/libs/common/libarccommon.la  $(top_srcdir)/src/hed/libs/message/libmessage.la


http_test_withtls_SOURCES=PayloadHTTP.cpp ../tcp/PayloadTCPSocket.cpp ../tls/PayloadTLSStream.cpp ../tls/PayloadTLSSocket.cpp  http_test_withtls.cpp
http_test_withtls_CPPFLAGS=$(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) $(OPENSSL_CFLAGS) -I$(top_srcdir)/src/libs/
http_test_withtls_LDFLAGS=$(LIBXML2_LIBS) $(OPENSSL_LIBS)
http_test_withtls_LDADD=$(top_srcdir)/src/libs/common/libarccommon.la  $(top_srcdir)/src/hed/libs/message/libmessage.la


