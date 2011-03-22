bin_PROGRAMS = saml_assertion_init

man_MANS = saml_assertion_init.1 

CLILIBS = \
	$(top_srcdir)/src/hed/libs/client/libarcclient.la \
	$(top_srcdir)/src/hed/libs/xmlsec/libarcxmlsec.la \
	$(top_srcdir)/src/hed/libs/credential/libarccredential.la \
	$(top_srcdir)/src/hed/libs/message/libarcmessage.la \
	$(top_srcdir)/src/hed/libs/loader/libarcloader.la \
	$(top_srcdir)/src/hed/libs/common/libarccommon.la

saml_assertion_init_SOURCES = saml_assertion_init.cpp
saml_assertion_init_CXXFLAGS = -I$(top_srcdir)/include \
	$(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) $(XMLSEC_CFLAGS)
saml_assertion_init_LDADD = $(CLILIBS) $(GLIBMM_LIBS)
