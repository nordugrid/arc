pkglib_LTLIBRARIES = libmcchttp.la
noinst_PROGRAMS = http_test http_test_withtls

libmcchttp_la_SOURCES  = MCCHTTP.cpp PayloadHTTP.cpp MCCHTTP.h PayloadHTTP.h
libmcchttp_la_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
libmcchttp_la_LIBADD   = $(top_srcdir)/src/hed/libs/message/libarcmessage.la $(top_srcdir)/src/hed/libs/common/libarccommon.la
libmcchttp_la_LDFLAGS  = $(LIBXML2_LIBS) -no-undefined -avoid-version -module

http_test_SOURCES  = http_test.cpp
http_test_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
http_test_LDFLAGS  = $(LIBXML2_LIBS)
http_test_LDADD    = $(top_srcdir)/src/hed/libs/common/libarccommon.la $(top_srcdir)/src/hed/libs/message/libarcmessage.la $(top_srcdir)/src/hed/libs/loader/libarcloader.la

http_test_withtls_SOURCES  = http_test_withtls.cpp
http_test_withtls_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) $(OPENSSL_CFLAGS) -I$(top_srcdir)/include 
http_test_withtls_LDFLAGS  = $(LIBXML2_LIBS) $(OPENSSL_LIBS)
http_test_withtls_LDADD    = $(top_srcdir)/src/hed/libs/common/libarccommon.la $(top_srcdir)/src/hed/libs/message/libarcmessage.la $(top_srcdir)/src/hed/libs/loader/libarcloader.la
