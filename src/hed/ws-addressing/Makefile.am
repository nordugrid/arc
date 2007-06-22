noinst_LTLIBRARIES= libwsaddressing.la
noinst_PROGRAMS=test

libwsaddressing_ladir=$(includedir)
libwsaddressing_la_HEADERS=WSA.h
libwsaddressing_la_SOURCES=WSA.cpp
libwsaddressing_la_CPPFLAGS=$(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/src/libs/
libwsaddressing_la_LIBADD=$(top_srcdir)/src/libs/common/libarccommon.la
libwsaddressing_la_LDFLAGS=$(LIBXML2_LIBS)

test_SOURCES=test.cpp WSA.cpp 
test_CPPFLAGS=$(LIBXML2_CFLAGS) -I$(top_srcdir)/src/libs/
test_LDFLAGS=$(LIBXML2_LIBS) 
test_LDADD=$(top_srcdir)/src/libs/common/libarccommon.la $(top_srcdir)/src/hed/libs/message/libmessage.la
