noinst_LTLIBRARIES= libwsaddressing.la
noinst_PROGRAMS=test

libwsaddressing_ladir = $(pkgincludedir)/ws-addressing
libwsaddressing_la_HEADERS  = WSA.h
libwsaddressing_la_SOURCES  = WSA.cpp
libwsaddressing_la_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
libwsaddressing_la_LIBADD   = $(top_srcdir)/src/hed/libs/common/libarccommon.la $(top_srcdir)/src/hed/libs/message/libarcmessage.la
libwsaddressing_la_LDFLAGS  = $(LIBXML2_LIBS)

test_SOURCES  = test.cpp 
test_CXXFLAGS = $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
test_LDFLAGS  = $(LIBXML2_LIBS) 
test_LDADD    = libwsaddressing.la $(top_srcdir)/src/hed/libs/common/libarccommon.la $(top_srcdir)/src/hed/libs/message/libarcmessage.la
