SUBDIRS=schema
noinst_LTLIBRARIES = libsamltokensh.la
libsamltokensh_la_SOURCES  = SAMLTokenSH.cpp SAMLTokenSH.h
libsamltokensh_la_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) $(XMLSEC_OPENSSL_CFLAGS) $(XMLSEC_CFLAGS) -I$(top_srcdir)/include
libsamltokensh_la_LIBADD   = $(top_srcdir)/src/hed/libs/message/libarcmessage.la $(top_srcdir)/src/hed/libs/ws-security/libarcwssecurity.la $(top_srcdir)/src/hed/libs/common/libarccommon.la $(top_srcdir)/src/hed/libs/xmlsec/libarcxmlsec.la $(top_srcdir)/src/hed/libs/credential/libarccredential.la $(XMLSEC_OPENSSL_LIBS) $(XMLSEC_LIBS)
libsamltokensh_la_LDFLAGS  = -no-undefined

