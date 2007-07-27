pkglib_LTLIBRARIES = libmccsoap.la
noinst_PROGRAMS=test

libmccsoap_ladir=$(includedir)
libmccsoap_la_HEADERS=MCCSOAP.h
libmccsoap_la_SOURCES=MCCSOAP.cpp
libmccsoap_la_CPPFLAGS=$(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/src/libs -I$(top_srcdir)/src/hed/libs
libmccsoap_la_LIBADD=$(top_srcdir)/src/hed/libs/message/libmessage.la $(top_srcdir)/src/hed/libs/ws-addressing/libwsaddressing.la
libmccsoap_la_LDFLAGS=$(LIBXML2_LIBS) -no-undefined -avoid-version -module

test_SOURCES=test.cpp
test_CPPFLAGS=$(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/src/libs
test_LDFLAGS=$(LIBXML2_LIBS)
test_LDADD=libmccsoap.la
