lib_LTLIBRARIES = libarccredential.la
noinst_PROGRAMS = testproxy testcertinfo testproxy2proxy testvoms testeec

#if WIN32
#VOMS_HEADER =
#VOMS_SOURCE =
#else
VOMS_HEADER = VOMSUtil.h
VOMS_SOURCE = VOMSUtil.cpp
#endif

libarccredential_ladir = $(pkgincludedir)/credential
libarccredential_la_HEADERS  = Credential.h CertUtil.h Proxycertinfo.h \
                               VOMSAttribute.h $(VOMS_HEADER)
libarccredential_la_SOURCES  = Proxycertinfo.cpp CertUtil.cpp \
                               Credential.cpp listfunc.c listfunc.h \
                               VOMSAttribute.cpp $(VOMS_SOURCE)
libarccredential_la_CXXFLAGS = -I$(top_srcdir)/include \
	$(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) $(OPENSSL_CFLAGS)
libarccredential_la_LIBADD = \
	$(top_srcdir)/src/hed/libs/crypto/libarccrypto.la \
	$(top_srcdir)/src/hed/libs/message/libarcmessage.la \
	$(top_srcdir)/src/hed/libs/common/libarccommon.la \
	$(GLIBMM_LIBS) $(OPENSSL_LIBS)
libarccredential_la_LDFLAGS  = -version-info 1:0:0

testproxy_SOURCES = testproxy.cpp
testproxy_CXXFLAGS = -I$(top_srcdir)/include \
	$(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) $(OPENSSL_CFLAGS)
testproxy_LDADD = ./libarccredential.la \
	$(top_srcdir)/src/hed/libs/common/libarccommon.la \
	$(GLIBMM_LIBS) $(LIBXML2_LIBS) $(OPENSSL_LIBS) $(EXTRA_LIBS)

testcertinfo_SOURCES = testcertinfo.cpp
testcertinfo_CXXFLAGS = -I$(top_srcdir)/include \
	$(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) $(OPENSSL_CFLAGS)
testcertinfo_LDADD = ./libarccredential.la \
	$(top_srcdir)/src/hed/libs/common/libarccommon.la \
	$(GLIBMM_LIBS) $(LIBXML2_LIBS) $(OPENSSL_LIBS) $(EXTRA_LIBS)

testproxy2proxy_SOURCES = testproxy2proxy.cpp
testproxy2proxy_CXXFLAGS = -I$(top_srcdir)/include \
	$(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) $(OPENSSL_CFLAGS)
testproxy2proxy_LDADD = ./libarccredential.la \
	$(top_srcdir)/src/hed/libs/common/libarccommon.la \
	$(GLIBMM_LIBS) $(LIBXML2_LIBS) $(OPENSSL_LIBS) $(EXTRA_LIBS)

testvoms_SOURCES = testvoms.cpp
testvoms_CXXFLAGS = -I$(top_srcdir)/include \
	$(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) $(OPENSSL_CFLAGS)
testvoms_LDADD = ./libarccredential.la \
	$(top_srcdir)/src/hed/libs/common/libarccommon.la \
	$(GLIBMM_LIBS) $(LIBXML2_LIBS) $(OPENSSL_LIBS) $(EXTRA_LIBS)

testeec_SOURCES = testeec.cpp
testeec_CXXFLAGS = -I$(top_srcdir)/include \
	$(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) $(OPENSSL_CFLAGS)
testeec_LDADD = ./libarccredential.la \
	$(top_srcdir)/src/hed/libs/common/libarccommon.la \
	$(GLIBMM_LIBS) $(LIBXML2_LIBS) $(OPENSSL_LIBS) $(EXTRA_LIBS)

DIST_SUBDIRS = test
SUBDIRS = $(TEST_DIR)
