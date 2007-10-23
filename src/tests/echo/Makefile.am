pkglib_LTLIBRARIES = libecho.la
noinst_PROGRAMS=test test_client test_service
bin_PROGRAMS=perftest
SOURCES =

libecho_la_SOURCES=echo.cpp echo.h
libecho_la_CXXFLAGS=$(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
libecho_la_LDFLAGS=$(top_srcdir)/src/hed/libs/common/libarccommon.la $(top_srcdir)/src/hed/libs/loader/libarcloader.la $(top_srcdir)/src/hed/libs/message/libmessage.la -no-undefined -avoid-version -module

test_SOURCES=$(SOURCES) test.cpp
test_CXXFLAGS=$(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
test_LDFLAGS=$(GLIBMM_LIBS) $(LIBXML2_LIBS)
test_LDADD=$(top_srcdir)/src/hed/libs/message/libmessage.la $(top_srcdir)/src/hed/libs/common/libarccommon.la $(top_srcdir)/src/hed/libs/loader/libarcloader.la $(top_srcdir)/src/hed/libs/security/libsecurity.la $(top_srcdir)/src/hed/mcc/tls/libmcctls.la $(top_srcdir)/src/hed/mcc/tcp/libmcctcp.la $(top_srcdir)/src/hed/mcc/http/libmcchttp.la $(top_srcdir)/src/hed/mcc/soap/libmccsoap.la $(top_srcdir)/src/hed/libs/misc/libarcclient.la libecho.la

test_client_SOURCES=$(SOURCES) test_client.cpp
test_client_CXXFLAGS=$(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
test_client_LDFLAGS=$(GLIBMM_LIBS) $(LIBXML2_LIBS)
test_client_LDADD=$(top_srcdir)/src/hed/libs/common/libarccommon.la $(top_srcdir)/src/hed/libs/loader/libarcloader.la $(top_srcdir)/src/hed/libs/message/libmessage.la

test_service_SOURCES=$(SOURCES) test_service.cpp
test_service_CXXFLAGS=$(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
test_service_LDFLAGS=$(GLIBMM_LIBS) $(LIBXML2_LIBS)
test_service_LDADD=$(top_srcdir)/src/hed/libs/common/libarccommon.la $(top_srcdir)/src/hed/libs/loader/libarcloader.la $(top_srcdir)/src/hed/libs/message/libmessage.la

perftest_SOURCES=$(SOURCES) perftest.cpp
perftest_CXXFLAGS=$(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
perftest_LDFLAGS=$(GLIBMM_LIBS) $(LIBXML2_LIBS)
perftest_LDADD=$(top_srcdir)/src/hed/libs/common/libarccommon.la $(top_srcdir)/src/hed/libs/loader/libarcloader.la $(top_srcdir)/src/hed/libs/message/libmessage.la
