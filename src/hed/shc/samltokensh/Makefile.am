SUBDIRS = schema

noinst_LTLIBRARIES = libsamltokensh.la

libsamltokensh_la_SOURCES = SAMLTokenSH.cpp SAMLTokenSH.h

libsamltokensh_la_CXXFLAGS = -I$(top_srcdir)/include \
	$(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) \
	$(XMLSEC_OPENSSL_CFLAGS) $(XMLSEC_CFLAGS)
libsamltokensh_la_LIBADD = \
	$(top_srcdir)/src/hed/libs/ws-security/libarcwssecurity.la \
	$(top_srcdir)/src/hed/libs/xmlsec/libarcxmlsec.la \
	$(top_srcdir)/src/hed/libs/credential/libarccredential.la \
	$(top_srcdir)/src/hed/libs/message/libarcmessage.la \
	$(top_srcdir)/src/hed/libs/common/libarccommon.la \
	$(XMLSEC_OPENSSL_LIBS) $(XMLSEC_LIBS)
