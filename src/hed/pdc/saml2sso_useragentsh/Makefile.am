
noinst_LTLIBRARIES = libsaml2ssouseragentsh.la
libsaml2ssouseragentsh_la_SOURCES  = SAML2SSO_UserAgentSH.cpp SAML2SSO_UserAgentSH.h
libsaml2ssouseragentsh_la_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) $(XMLSEC_OPENSSL_CFLAGS) $(XMLSEC_CFLAGS) -I$(top_srcdir)/include
libsaml2ssouseragentsh_la_LIBADD   = $(top_srcdir)/src/hed/libs/message/libarcmessage.la $(top_srcdir)/src/hed/libs/ws-security/libarcwssecurity.la $(top_srcdir)/src/hed/libs/common/libarccommon.la $(top_srcdir)/src/hed/libs/xmlsec/libarcxmlsec.la $(XMLSEC_OPENSSL_LIBS) $(XMLSEC_LIBS)
libsaml2ssouseragentsh_la_LDFLAGS  = -no-undefined

