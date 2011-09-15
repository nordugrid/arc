# Makefile to create the client for the Webservice (./interfaces/client)
if JANITOR_SERVICE_ENABLED

bin_PROGRAMS = janitorClient

janitorClient_SOURCES = janitorClient.cpp
janitorClient_CXXFLAGS = -I$(top_srcdir)/include \
	$(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS)
janitorClient_LDADD = \
	$(top_builddir)/src/hed/libs/client/libarcclient.la \
	$(top_builddir)/src/hed/libs/message/libarcmessage.la \
	$(top_builddir)/src/hed/libs/loader/libarcloader.la \
	$(top_builddir)/src/hed/libs/common/libarccommon.la \
	$(GLIBMM_LIBS) $(LIBXML2_LIBS)

endif 
