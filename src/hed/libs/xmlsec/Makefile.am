lib_LTLIBRARIES = libarcxmlsec.la
libarcxmlsec_ladir       = $(pkgincludedir)/xmlsec
libarcxmlsec_la_HEADERS  =  XmlSecUtils.h XMLSecNode.h saml_util.h

libarcxmlsec_la_SOURCES  = XmlSecUtils.cpp XMLSecNode.cpp saml_util.cpp
libarcxmlsec_la_CXXFLAGS = $(LIBXML2_CFLAGS) $(GLIBMM_CFLAGS) $(OPENSSL_CFLAGS) $(XMLSEC_OPENSSL_CFLAGS) $(XMLSEC_CFAGS) -I$(top_srcdir)/include
libarcxmlsec_la_LIBADD   = $(top_srcdir)/src/hed/libs/common/libarccommon.la \
                           $(top_srcdir)/src/hed/libs/message/libarcmessage.la \
                           $(LIBXML2_LIBS) $(GLIBMM_LIBS) $(OPENSSL_LIBS) $(XMLSEC_OPENSSL_LIBS) $(XMLSEC_LIBS)

noinst_PROGRAMS = test_xmlsecnode

test_xmlsecnode_SOURCES=test_xmlsecnode.cpp
test_xmlsecnode_CXXFLAGS=$(LIBXML2_CFLAGS) $(GLIBMM_CFLAGS) $(XMLSEC_CFLAGS)  -I$(top_srcdir)/include
test_xmlsecnode_LDFLAGS=$(LIBXML2_LIBS) $(XMLSEC_LIBS) $(EXTRA_LIBS)
test_xmlsecnode_LDADD=./libarcxmlsec.la $(top_srcdir)/src/hed/libs/common/libarccommon.la $(top_srcdir)/src/hed/libs/message/libarcmessage.la

