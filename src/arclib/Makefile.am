lib_LTLIBRARIES = libcredential.la
noinst_PROGRAMS = test  testcertinfo testproxy2proxy 

libcredential_ladir = $(pkgincludedir)/arclib
libcredential_la_HEADERS  = Credential.h
libcredential_la_SOURCES  = Proxycertinfo.cpp Proxycertinfo.h cert_util.cpp cert_util.h Credential.cpp listfunc.c listfunc.h \
                            VOMSAttribute.c VOMSAttribute.h voms_util.cpp voms_util.h
libcredential_la_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) $(OPENSSL_CFLAGS) -I$(top_srcdir)/include
libcredential_la_LIBADD   = $(top_srcdir)/src/hed/libs/common/libarccommon.la $(top_srcdir)/src/hed/libs/message/libarcmessage.la $(OPENSSL_LIBS)

test_SOURCES  = test.cpp
test_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
test_LDFLAGS  = $(GLIBMM_LIBS) $(EXTRA_LIBS)
test_LDADD    = ./libcredential.la

testcertinfo_SOURCES  = testcertinfo.cpp
testcertinfo_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
testcertinfo_LDFLAGS  = $(GLIBMM_LIBS) $(EXTRA_LIBS)
testcertinfo_LDADD    = ./libcredential.la

testproxy2proxy_SOURCES  = testproxy2proxy.cpp
testproxy2proxy_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
testproxy2proxy_LDFLAGS  = $(GLIBMM_LIBS) $(EXTRA_LIBS)
testproxy2proxy_LDADD    = ./libcredential.la

