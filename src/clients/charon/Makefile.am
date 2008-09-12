bin_PROGRAMS = arcdecision
doc_DATA = charon_client.xml.example charon_request.xml.example

man_MANS = arcdecision.1 

CLILIBS = $(top_srcdir)/src/hed/libs/common/libarccommon.la \
          $(top_srcdir)/src/hed/libs/client/libarcclient.la \
          $(top_srcdir)/src/hed/libs/data/libarcdata2.la \
          $(top_srcdir)/src/hed/libs/loader/libarcloader.la \
          $(top_srcdir)/src/hed/libs/message/libarcmessage.la \
          $(top_srcdir)/src/hed/libs/security/libarcsecurity.la

arcdecision_SOURCES  = arcdecision.cpp
arcdecision_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
arcdecision_LDADD    = $(CLILIBS)

EXTRA_DIST = $(doc_DATA)
