noinst_LTLIBRARIES = libwssecurity.la
noinst_PROGRAMS = test test_x509token

libwssecurity_ladir = $(pkgincludedir)/ws-security
libwssecurity_la_HEADERS  = UsernameToken.h X509Token.h
libwssecurity_la_SOURCES  = UsernameToken.cpp X509Token.cpp
libwssecurity_la_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) $(OPENSSL_CFLAGS) $(XMLSEC_CFLAGS) -I$(top_srcdir)/include
libwssecurity_la_LIBADD   = $(top_srcdir)/src/hed/libs/common/libarccommon.la $(top_srcdir)/src/hed/libs/message/libarcmessage.la $(OPENSSL_LIBS) $(XMLSEC_LIBS)
#libwssecurity_la_LDFLAGS  = $(LIBXML2_LIBS)

test_SOURCES  = test.cpp 
test_CXXFLAGS = $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
test_LDFLAGS  = $(LIBXML2_LIBS)
test_LDADD    = libwssecurity.la $(top_srcdir)/src/hed/libs/common/libarccommon.la $(top_srcdir)/src/hed/libs/message/libarcmessage.la

test_x509token_SOURCES  = test_x509token.cpp
test_x509token_CXXFLAGS = $(LIBXML2_CFLAGS) $(XMLSEC_CFLAGS) -I$(top_srcdir)/include
test_x509token_LDFLAGS  = $(LIBXML2_LIBS) $(OPENSSL_LIBS) $(XMLSEC_LIBS)
test_x509token_LDADD    = libwssecurity.la $(top_srcdir)/src/hed/libs/common/libarccommon.la $(top_srcdir)/src/hed/libs/message/libarcmessage.la

