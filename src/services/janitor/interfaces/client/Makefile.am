# Makefile to create the client for the Webservice (./interfaces/client)
if JANITOR_SERVICE_ENABLED



bin_PROGRAMS = janitorClient

janitorClient_SOURCES  = janitorClient.cpp
janitorClient_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include 
janitorClient_LDADD    = $(top_srcdir)/src/hed/libs/common/libarccommon.la \
	$(top_srcdir)/src/hed/libs/client/libarcclient.la \
	$(top_srcdir)/src/hed/libs/loader/libarcloader.la \
	$(top_srcdir)/src/hed/libs/message/libarcmessage.la \
	$(GLIBMM_LIBS) $(LIBXML2_LIBS)

CLEANFILES = $(bin_PROGRAMS)

endif 

# see ../interface/commandline/Makefile.am