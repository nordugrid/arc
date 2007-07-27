pkglib_LTLIBRARIES = libmcctcp.la
noinst_PROGRAMS=test
SOURCES = MCCTCP.cpp PayloadTCPSocket.cpp

libmcctcp_ladir=$(includedir)
libmcctcp_la_HEADERS=MCCTCP.h PayloadTCPSocket.h
libmcctcp_la_SOURCES=MCCTCP.cpp PayloadTCPSocket.cpp
libmcctcp_la_CPPFLAGS=$(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/src/libs -I$(top_srcdir)/src/hed/libs
libmcctcp_la_LIBADD=$(top_srcdir)/src/hed/libs/message/libmessage.la
libmcctcp_la_LDFLAGS=$(LIBXML2_LIBS) -lpthread -no-undefined -avoid-version -module

test_SOURCES=$(SOURCES) test.cpp
test_CPPFLAGS=$(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/src/libs -I$(top_srcdir)/src/hed/libs
test_LDFLAGS=$(LIBXML2_LIBS) -lpthread
test_LDADD=$(top_srcdir)/src/libs/common/libarccommon.la $(top_srcdir)/src/hed/libs/message/libmessage.la
