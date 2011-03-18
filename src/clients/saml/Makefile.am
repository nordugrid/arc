bin_PROGRAMS = saml_assertion_init
doc_DATA =

man_MANS = saml_assertion_init.1 

CLILIBS = $(top_srcdir)/src/hed/libs/common/libarccommon.la \
          $(top_srcdir)/src/hed/libs/client/libarcclient.la \
          $(top_srcdir)/src/hed/libs/loader/libarcloader.la \
          $(top_srcdir)/src/hed/libs/message/libarcmessage.la \
          $(top_srcdir)/src/hed/libs/xmlsec/libarcxmlsec.la \
          $(top_srcdir)/src/hed/libs/credential/libarccredential.la

saml_assertion_init_SOURCES  = saml_assertion_init.cpp
saml_assertion_init_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) $(XMLSEC_CFLAGS) -I$(top_srcdir)/include
saml_assertion_init_LDADD    = $(CLILIBS) $(GLIBMM_LIBS)

EXTRA_DIST = $(doc_DATA)
