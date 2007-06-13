lib_LTLIBRARIES= libecho.la
noinst_PROGRAMS=test test_client perftest
SOURCES =

libecho_ladir=$(includedir)
libecho_la_HEADERS=echo.h
libecho_la_SOURCES=echo.cpp
libecho_la_CPPFLAGS=$(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/src/libs/

test_SOURCES=$(SOURCES) test.cpp
test_CPPFLAGS=$(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/src/libs/
test_LDFLAGS=$(GLIBMM_LIBS) $(LIBXML2_LIBS)
test_LDADD=$(top_srcdir)/src/libs/common/libarccommon.la $(top_srcdir)/src/hed/libs/loader/libarcloader.la $(top_srcdir)/src/hed/libs/message/libmessage.la

test_client_SOURCES=$(SOURCES) test_client.cpp
test_client_CPPFLAGS=$(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/src/libs/
test_client_LDFLAGS=$(GLIBMM_LIBS) $(LIBXML2_LIBS)
test_client_LDADD=$(top_srcdir)/src/libs/common/libarccommon.la $(top_srcdir)/src/hed/libs/loader/libarcloader.la $(top_srcdir)/src/hed/libs/message/libmessage.la

perftest_SOURCES=$(SOURCES) perftest.cpp
perftest_CPPFLAGS=$(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/src/libs/
perftest_LDFLAGS=$(GLIBMM_LIBS) $(LIBXML2_LIBS)
perftest_LDADD=$(top_srcdir)/src/libs/common/libarccommon.la $(top_srcdir)/src/hed/libs/loader/libarcloader.la $(top_srcdir)/src/hed/libs/message/libmessage.la
