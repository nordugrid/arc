noinst_LTLIBRARIES = libcredential.la
noinst_PROGRAMS = test  testcertinfo #testproxy2proxy

libcredential_ladir = $(pkgincludedir)/arclib
libcredential_la_HEADERS  = Proxycertinfo.h  cert_util.h Credential.h listfunc.h VOMSAttribute.h voms_util.h
libcredential_la_SOURCES  = Proxycertinfo.cpp cert_util.cpp Credential.cpp listfunc.c VOMSAttribute.c voms_util.cpp
libcredential_la_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) $(OPENSSL_CFLAGS) -I$(top_srcdir)/include
libcredential_la_LIBADD   = $(top_srcdir)/src/hed/libs/message/libarcmessage.la $(OPENSSL_LIBS)

test_SOURCES  = test.cpp
test_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
test_LDFLAGS  = $(GLIBMM_LIBS) $(EXTRA_LIBS)
test_LDADD    = libcredential.la

#testproxy2proxy_SOURCES  = testproxy2proxy.cpp
#testproxy2proxy_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
#testproxy2proxy_LDFLAGS  = $(GLIBMM_LIBS) $(EXTRA_LIBS)
#testproxy2proxy_LDADD    = libcredential.la

testcertinfo_SOURCES  = testcertinfo.cpp
testcertinfo_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
testcertinfo_LDFLAGS  = $(GLIBMM_LIBS) $(EXTRA_LIBS)
testcertinfo_LDADD    = libcredential.la

