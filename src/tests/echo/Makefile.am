pkglib_LTLIBRARIES = libecho.la
noinst_PROGRAMS=test test_client test_clientinterface test_service
bin_PROGRAMS=perftest
docdir = @docdir@
doc_DATA = echo.xml echo.wsdl

libecho_la_SOURCES  = echo.cpp echo.h
libecho_la_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
libecho_la_LIBADD   = $(top_srcdir)/src/hed/libs/common/libarccommon.la $(top_srcdir)/src/hed/libs/loader/libarcloader.la $(top_srcdir)/src/hed/libs/message/libarcmessage.la $(top_srcdir)/src/hed/libs/ws/libarcws.la $(LIBXML2_LIBS)
libecho_la_LDFLAGS  = -no-undefined -avoid-version -module

test_SOURCES =  test.cpp
test_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
test_LDFLAGS  = $(GLIBMM_LIBS) $(LIBXML2_LIBS)
test_LDADD    = $(top_srcdir)/src/hed/libs/common/libarccommon.la $(top_srcdir)/src/hed/libs/message/libarcmessage.la $(top_srcdir)/src/hed/libs/loader/libarcloader.la $(top_srcdir)/src/hed/libs/client/libarcclient.la $(top_srcdir)/src/hed/libs/data/libarcdata2.la

test_client_SOURCES  = test_client.cpp
test_client_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
test_client_LDFLAGS  = $(GLIBMM_LIBS) $(LIBXML2_LIBS)
test_client_LDADD    = $(top_srcdir)/src/hed/libs/common/libarccommon.la $(top_srcdir)/src/hed/libs/loader/libarcloader.la $(top_srcdir)/src/hed/libs/message/libarcmessage.la

test_clientinterface_SOURCES  = test_clientinterface.cpp
test_clientinterface_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
test_clientinterface_LDFLAGS  = $(GLIBMM_LIBS) $(LIBXML2_LIBS)
test_clientinterface_LDADD    = $(top_srcdir)/src/hed/libs/common/libarccommon.la $(top_srcdir)/src/hed/libs/loader/libarcloader.la $(top_srcdir)/src/hed/libs/message/libarcmessage.la $(top_srcdir)/src/hed/libs/client/libarcclient.la $(top_srcdir)/src/hed/libs/data/libarcdata2.la

test_service_SOURCES  = test_service.cpp
test_service_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
test_service_LDFLAGS  = $(GLIBMM_LIBS) $(LIBXML2_LIBS)
test_service_LDADD    = $(top_srcdir)/src/hed/libs/common/libarccommon.la $(top_srcdir)/src/hed/libs/loader/libarcloader.la $(top_srcdir)/src/hed/libs/message/libarcmessage.la

perftest_SOURCES  = perftest.cpp
perftest_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
perftest_LDFLAGS  = $(GLIBMM_LIBS) $(LIBXML2_LIBS)
perftest_LDADD    = $(top_srcdir)/src/hed/libs/common/libarccommon.la $(top_srcdir)/src/hed/libs/loader/libarcloader.la $(top_srcdir)/src/hed/libs/message/libarcmessage.la

EXTRA_DIST = $(doc_DATA)

man_MANS = perftest.1
