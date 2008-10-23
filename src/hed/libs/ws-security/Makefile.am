lib_LTLIBRARIES = libarcwssecurity.la
#noinst_LTLIBRARIES = libwssecurity.la
if XMLSEC_ENABLED
noinst_PROGRAMS = test test_x509token test_samltoken
else 
noinst_PROGRAMS = test
endif

libarcwssecurity_ladir = $(pkgincludedir)/ws-security
if XMLSEC_ENABLED
libarcwssecurity_la_HEADERS  = UsernameToken.h X509Token.h SAMLToken.h 
libarcwssecurity_la_SOURCES  = UsernameToken.cpp X509Token.cpp SAMLToken.cpp
else
libarcwssecurity_la_HEADERS  = UsernameToken.h
libarcwssecurity_la_SOURCES  = UsernameToken.cpp
endif
libarcwssecurity_la_CXXFLAGS = $(LIBXML2_CFLAGS) $(GLIBMM_CFLAGS) $(OPENSSL_CFLAGS) $(XMLSEC_CFLAGS) -I$(top_srcdir)/include
libarcwssecurity_la_LIBADD   = $(top_srcdir)/src/hed/libs/common/libarccommon.la $(top_srcdir)/src/hed/libs/message/libarcmessage.la \
                            $(LIBXML2_LIBS) $(GLIBMM_LIBS) $(OPENSSL_LIBS) $(XMLSEC_LIBS)
#libarcwssecurity_la_LDFLAGS  = $(LIBXML2_LIBS)

test_SOURCES  = test.cpp 
test_CXXFLAGS = $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
test_LDFLAGS  = $(LIBXML2_LIBS)
test_LDADD    = libarcwssecurity.la $(top_srcdir)/src/hed/libs/common/libarccommon.la $(top_srcdir)/src/hed/libs/message/libarcmessage.la

if XMLSEC_ENABLED
test_x509token_SOURCES  = test_x509token.cpp
test_x509token_CXXFLAGS = $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
test_x509token_LDFLAGS  = $(LIBXML2_LIBS) $(OPENSSL_LIBS)
test_x509token_LDADD    = libarcwssecurity.la $(top_srcdir)/src/hed/libs/common/libarccommon.la $(top_srcdir)/src/hed/libs/message/libarcmessage.la

test_samltoken_SOURCES  = test_samltoken.cpp
test_samltoken_CXXFLAGS = $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
test_samltoken_LDFLAGS  = $(LIBXML2_LIBS) $(OPENSSL_LIBS)
test_samltoken_LDADD    = libarcwssecurity.la $(top_srcdir)/src/hed/libs/common/libarccommon.la $(top_srcdir)/src/hed/libs/message/libarcmessage.la
else
endif
