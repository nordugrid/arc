lib_LTLIBRARIES = libarccredential.la
noinst_PROGRAMS = testproxy testcertinfo testproxy2proxy testvoms

if XMLSEC_ENABLED
SAML_UTIL_HEADER = saml_util.h
SAML_UTIL_SOURCE = saml_util.cpp
else
SAML_UTIL_HEADER =
SAML_UTIL_SOURCE =
endif

if WIN32
VOMS_HEADER =
VOMS_SOURCE =
else
VOMS_HEADER = voms_util.h
VOMS_SOURCE = voms_util.cpp
endif

libarccredential_ladir = $(pkgincludedir)/arclib
libarccredential_la_HEADERS  = Credential.h $(VOMS_HEADER) $(SAML_UTIL_HEADER)
libarccredential_la_SOURCES  = Proxycertinfo.cpp Proxycertinfo.h cert_util.cpp cert_util.h \
                               Credential.cpp listfunc.c listfunc.h \
                               VOMSAttribute.cpp VOMSAttribute.h $(VOMS_SOURCE) $(SAML_UTIL_SOURCE)
libarccredential_la_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) $(XMLSEC_CFLAGS) $(OPENSSL_CFLAGS) -I$(top_srcdir)/include
libarccredential_la_LIBADD   = $(top_srcdir)/src/hed/libs/common/libarccommon.la $(top_srcdir)/src/hed/libs/message/libarcmessage.la $(GLIBMM_LIBS) $(XMLSEC_LIBS) $(OPENSSL_LIBS)

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

