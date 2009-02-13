lib_LTLIBRARIES = libarccredential.la
noinst_PROGRAMS = testproxy testcertinfo testproxy2proxy testvoms testeec

if WIN32
VOMS_HEADER =
VOMS_SOURCE =
else
VOMS_HEADER = VOMSUtil.h
VOMS_SOURCE = VOMSUtil.cpp
endif

libarccredential_ladir = $(pkgincludedir)/credential
libarccredential_la_HEADERS  = Credential.h CertUtil.h Proxycertinfo.h VOMSAttribute.h $(VOMS_HEADER)
libarccredential_la_SOURCES  = Proxycertinfo.cpp CertUtil.cpp \
                               Credential.cpp listfunc.c listfunc.h \
                               VOMSAttribute.cpp $(VOMS_SOURCE)
libarccredential_la_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) $(OPENSSL_CFLAGS) -I$(top_srcdir)/include
libarccredential_la_LIBADD   = $(top_srcdir)/src/hed/libs/common/libarccommon.la $(top_srcdir)/src/hed/libs/message/libarcmessage.la $(GLIBMM_LIBS) $(OPENSSL_LIBS)

testproxy_SOURCES  = testproxy.cpp
testproxy_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) $(OPENSSL_CFLAGS) -I$(top_srcdir)/include
testproxy_LDFLAGS  = $(GLIBMM_LIBS) $(LIBXML2_LIBS) $(OPENSSL_LIBS) $(EXTRA_LIBS)
testproxy_LDADD    = ./libarccredential.la

testcertinfo_SOURCES  = testcertinfo.cpp
testcertinfo_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) $(OPENSSL_CFLAGS) -I$(top_srcdir)/include
testcertinfo_LDFLAGS  = $(GLIBMM_LIBS) $(LIBXML2_LIBS) $(OPENSSL_LIBS) $(EXTRA_LIBS)
testcertinfo_LDADD    = ./libarccredential.la

testproxy2proxy_SOURCES  = testproxy2proxy.cpp
testproxy2proxy_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) $(OPENSSL_CFLAGS) -I$(top_srcdir)/include
testproxy2proxy_LDFLAGS  = $(GLIBMM_LIBS) $(LIBXML2_LIBS) $(OPENSSL_LIBS) $(EXTRA_LIBS)
testproxy2proxy_LDADD    = ./libarccredential.la

testvoms_SOURCES  = testvoms.cpp
testvoms_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) $(OPENSSL_CFLAGS) -I$(top_srcdir)/include
testvoms_LDFLAGS  = $(GLIBMM_LIBS) $(LIBXML2_LIBS) $(OPENSSL_LIBS) $(EXTRA_LIBS)
testvoms_LDADD    = ./libarccredential.la

testeec_SOURCES  = testeec.cpp
testeec_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) $(OPENSSL_CFLAGS) -I$(top_srcdir)/include
testeec_LDFLAGS  = $(GLIBMM_LIBS) $(LIBXML2_LIBS) $(OPENSSL_LIBS) $(EXTRA_LIBS)
testeec_LDADD    = ./libarccredential.la

