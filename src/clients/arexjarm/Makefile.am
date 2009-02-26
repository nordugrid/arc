bin_PROGRAMS = arexjarm

arexjarm_SOURCES  = arexjarm.cpp ArexLogParser.cpp LUTSClient.cpp

arexjarm_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include -ggdb
arexjarm_LDADD    = $(top_srcdir)/src/hed/libs/common/libarccommon.la \
	$(top_srcdir)/src/hed/libs/client/libarcclient.la \
	$(top_srcdir)/src/hed/libs/loader/libarcloader.la \
	$(top_srcdir)/src/hed/libs/message/libarcmessage.la \
	$(GLIBMM_LIBS) $(LIBXML2_LIBS)

dist_bin_SCRIPTS = arexjarm-wrapper.sh

#man_MANS = arexjarm.1
