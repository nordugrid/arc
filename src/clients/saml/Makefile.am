bin_PROGRAMS = saml_assertion_init

man_MANS = saml_assertion_init.1 

CLILIBS = \
	$(top_builddir)/src/hed/libs/client/libarcclient.la \
	$(top_builddir)/src/hed/libs/xmlsec/libarcxmlsec.la \
	$(top_builddir)/src/hed/libs/credential/libarccredential.la \
	$(top_builddir)/src/hed/libs/message/libarcmessage.la \
	$(top_builddir)/src/hed/libs/loader/libarcloader.la \
	$(top_builddir)/src/hed/libs/common/libarccommon.la

saml_assertion_init_SOURCES = saml_assertion_init.cpp
saml_assertion_init_CXXFLAGS = -I$(top_srcdir)/include \
	$(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) $(XMLSEC_CFLAGS)
saml_assertion_init_LDADD = $(CLILIBS) $(GLIBMM_LIBS)
