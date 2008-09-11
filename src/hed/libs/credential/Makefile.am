lib_LTLIBRARIES = libarccredential.la
noinst_PROGRAMS = test  testcertinfo testproxy2proxy 

libarccredential_ladir = $(pkgincludedir)/arclib
libarccredential_la_HEADERS  = Credential.h
libarccredential_la_SOURCES  = Proxycertinfo.cpp Proxycertinfo.h cert_util.cpp cert_util.h Credential.cpp listfunc.c listfunc.h \
                            VOMSAttribute.cpp VOMSAttribute.h voms_util.cpp voms_util.h
libarccredential_la_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) $(OPENSSL_CFLAGS) -I$(top_srcdir)/include
libarccredential_la_LIBADD   = $(top_srcdir)/src/hed/libs/common/libarccommon.la $(top_srcdir)/src/hed/libs/message/libarcmessage.la $(OPENSSL_LIBS)

test_SOURCES  = test.cpp
test_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) $(OPENSSL_CFLAGS) -I$(top_srcdir)/include
test_LDFLAGS  = $(GLIBMM_LIBS) $(LIBXML2_LIBS) $(OPENSSL_LIBS) $(EXTRA_LIBS)
test_LDADD    = ./libarccredential.la

testcertinfo_SOURCES  = testcertinfo.cpp
testcertinfo_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) $(OPENSSL_CFLAGS) -I$(top_srcdir)/include
testcertinfo_LDFLAGS  = $(GLIBMM_LIBS) $(LIBXML2_LIBS) $(OPENSSL_LIBS) $(EXTRA_LIBS)
testcertinfo_LDADD    = ./libarccredential.la

testproxy2proxy_SOURCES  = testproxy2proxy.cpp
testproxy2proxy_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) $(OPENSSL_CFLAGS) -I$(top_srcdir)/include
testproxy2proxy_LDFLAGS  = $(GLIBMM_LIBS) $(LIBXML2_LIBS) $(OPENSSL_LIBS) $(EXTRA_LIBS)
testproxy2proxy_LDADD    = ./libarccredential.la

