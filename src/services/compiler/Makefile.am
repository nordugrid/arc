pkglib_LTLIBRARIES = libcompiler.la
noinst_PROGRAMS=test_client

libcompiler_la_SOURCES  = arex_client.cpp arex_client.h compiler.cpp compiler.h
libcompiler_la_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
libcompiler_la_LIBADD   = $(top_srcdir)/src/hed/libs/common/libarccommon.la $(top_srcdir)/src/hed/libs/loader/libarcloader.la $(top_srcdir)/src/hed/libs/message/libarcmessage.la $(top_srcdir)/src/hed/libs/client/libarcclient.la $(top_srcdir)/src/hed/libs/security/libarcsecurity.la $(top_srcdir)/src/hed/libs/data/libarcdata2.la $(LIBXML2_LIBS) $(top_srcdir)/src/hed/libs/delegation/libarcdelegation.la
libcompiler_la_LDFLAGS  = -no-undefined -avoid-version -module $(GLIBMM_LIBS)

test_client_SOURCES  = test_client.cpp
test_client_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
test_client_LDFLAGS  = $(GLIBMM_LIBS) $(LIBXML2_LIBS)
test_client_LDADD    = $(top_srcdir)/src/hed/libs/common/libarccommon.la $(top_srcdir)/src/hed/libs/loader/libarcloader.la $(top_srcdir)/src/hed/libs/message/libarcmessage.la $(top_srcdir)/src/hed/libs/delegation/libarcdelegation.la
