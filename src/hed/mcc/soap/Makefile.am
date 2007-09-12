pkglib_LTLIBRARIES = libmccsoap.la
noinst_PROGRAMS=test

libmccsoap_la_SOURCES=MCCSOAP.cpp MCCSOAP.h
libmccsoap_la_CXXFLAGS=$(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
libmccsoap_la_LIBADD=$(top_srcdir)/src/hed/libs/common/libarccommon.la $(top_srcdir)/src/hed/libs/message/libmessage.la $(top_srcdir)/src/hed/libs/ws-addressing/libwsaddressing.la
libmccsoap_la_LDFLAGS=$(LIBXML2_LIBS) -no-undefined -avoid-version -module

test_SOURCES=test.cpp
test_CXXFLAGS=$(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
test_LDFLAGS=$(LIBXML2_LIBS)
test_LDADD=libmccsoap.la
