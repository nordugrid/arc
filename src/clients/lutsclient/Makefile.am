bin_PROGRAMS = arexlutsclient

arexlutsclient_SOURCES  = main.cpp arexlutsclient.h ArexLogParser.cpp ArexLogParser.h LUTSClient.cpp LUTSClient.h

arexlutsclient_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include -ggdb
arexlutsclient_LDADD    = $(top_srcdir)/src/hed/libs/common/libarccommon.la \
	$(top_srcdir)/src/hed/libs/loader/libarcloader.la \
	$(top_srcdir)/src/hed/libs/message/libarcmessage.la \
	$(GLIBMM_LIBS) $(LIBXML2_LIBS)

dist_bin_SCRIPTS = arexlutsclient-wrapper.sh

#man_MANS = arexlutsclient.1
