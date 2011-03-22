bin_PROGRAMS = arcdecision

exampledir = $(pkgdatadir)/examples/charon
example_DATA = charon_client.xml.example charon_request.xml.example

man_MANS = arcdecision.1 

CLILIBS = \
	$(top_srcdir)/src/hed/libs/client/libarcclient.la \
	$(top_srcdir)/src/hed/libs/security/libarcsecurity.la \
	$(top_srcdir)/src/hed/libs/data/libarcdata2.la \
	$(top_srcdir)/src/hed/libs/credential/libarccredential.la \
	$(top_srcdir)/src/hed/libs/message/libarcmessage.la \
	$(top_srcdir)/src/hed/libs/loader/libarcloader.la \
	$(top_srcdir)/src/hed/libs/common/libarccommon.la

arcdecision_SOURCES = arcdecision.cpp
arcdecision_CXXFLAGS = -I$(top_srcdir)/include \
	$(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS)
arcdecision_LDADD = $(CLILIBS) $(GLIBMM_LIBS)

EXTRA_DIST = $(example_DATA)
