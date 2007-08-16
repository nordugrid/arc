pkglib_LTLIBRARIES = libmcchttp.la
noinst_PROGRAMS=test http_test http_test_withtls

libmcchttp_ladir=$(includedir)
libmcchttp_la_HEADERS=MCCHTTP.h PayloadHTTP.h
libmcchttp_la_SOURCES=MCCHTTP.cpp PayloadHTTP.cpp
libmcchttp_la_CPPFLAGS=$(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/src/libs -I$(top_srcdir)/src/hed/libs
libmcchttp_la_LIBADD=$(top_srcdir)/src/hed/libs/message/libmessage.la
libmcchttp_la_LDFLAGS=$(LIBXML2_LIBS) -no-undefined -avoid-version -module

test_SOURCES=test.cpp
test_CPPFLAGS=$(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/src/libs -I$(top_srcdir)/src/hed/libs
test_LDFLAGS=$(LIBXML2_LIBS)
test_LDADD=libmcchttp.la $(top_srcdir)/src/libs/common/libarccommon.la  $(top_srcdir)/src/hed/libs/message/libmessage.la

http_test_SOURCES=http_test.cpp
http_test_CPPFLAGS=$(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/src/libs -I$(top_srcdir)/src/hed/libs
http_test_LDFLAGS=$(LIBXML2_LIBS)
http_test_LDADD=libmcchttp.la ../tcp/libmcctcp.la $(top_srcdir)/src/libs/common/libarccommon.la  $(top_srcdir)/src/hed/libs/message/libmessage.la

http_test_withtls_SOURCES=http_test_withtls.cpp
http_test_withtls_CPPFLAGS=$(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) $(OPENSSL_CFLAGS) -I$(top_srcdir)/src/libs -I$(top_srcdir)/src/hed/libs
http_test_withtls_LDFLAGS=$(LIBXML2_LIBS) $(OPENSSL_LIBS)
http_test_withtls_LDADD=libmcchttp.la ../tcp/libmcctcp.la ../tls/libmcctls.la $(top_srcdir)/src/libs/common/libarccommon.la  $(top_srcdir)/src/hed/libs/message/libmessage.la


