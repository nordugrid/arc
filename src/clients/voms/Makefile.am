bin_PROGRAMS = voms_assertion_init
doc_DATA =

man_MANS = voms_assertion_init.1 

CLILIBS = $(top_srcdir)/src/hed/libs/common/libarccommon.la \
          $(top_srcdir)/src/hed/libs/client/libarcclient.la \
          $(top_srcdir)/src/hed/libs/loader/libarcloader.la \
          $(top_srcdir)/src/hed/libs/message/libarcmessage.la \
          $(top_srcdir)/src/hed/libs/xmlsec/libarcxmlsec.la \
          $(top_srcdir)/src/hed/libs/credential/libarccredential.la

voms_assertion_init_SOURCES  = voms_assertion_init.cpp
voms_assertion_init_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) $(XMLSEC_CFLAGS) -I$(top_srcdir)/include
voms_assertion_init_LDADD    = $(CLILIBS)

EXTRA_DIST = $(doc_DATA)
