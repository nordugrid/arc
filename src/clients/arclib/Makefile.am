bin_PROGRAMS = arcsub arcget arcstat arcinfo arckill arcclean arccat arcsync \
	arcresub arcmigrate arcresume arcrenew arctest

man_MANS = arcsub.1 arcget.1 arcstat.1 arcinfo.1 arckill.1 arcclean.1 arccat.1 \
	arcresub.1 arcmigrate.1 arcsync.1 arcresume.1 arcrenew.1

CLILIBS = \
	$(top_srcdir)/src/hed/libs/client/libarcclient.la \
	$(top_srcdir)/src/hed/libs/data/libarcdata2.la \
	$(top_srcdir)/src/hed/libs/credential/libarccredential.la \
	$(top_srcdir)/src/hed/libs/message/libarcmessage.la \
	$(top_srcdir)/src/hed/libs/loader/libarcloader.la \
	$(top_srcdir)/src/hed/libs/common/libarccommon.la

arcsub_SOURCES  = arcsub.cpp
arcsub_CXXFLAGS = -I$(top_srcdir)/include $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS)
arcsub_LDADD    = $(CLILIBS) $(GLIBMM_LIBS)

arctest_SOURCES  = arctest.cpp
arctest_CXXFLAGS = -I$(top_srcdir)/include $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS)
arctest_LDADD    = $(CLILIBS) $(GLIBMM_LIBS)

arcget_SOURCES  = arcget.cpp
arcget_CXXFLAGS = -I$(top_srcdir)/include $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS)
arcget_LDADD    = $(CLILIBS) $(GLIBMM_LIBS)

arcstat_SOURCES  = arcstat.cpp
arcstat_CXXFLAGS = -I$(top_srcdir)/include $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS)
arcstat_LDADD    = $(CLILIBS) $(GLIBMM_LIBS)

arcinfo_SOURCES  = arcinfo.cpp
arcinfo_CXXFLAGS = -I$(top_srcdir)/include $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS)
arcinfo_LDADD    = $(CLILIBS) $(GLIBMM_LIBS)

arckill_SOURCES  = arckill.cpp
arckill_CXXFLAGS = -I$(top_srcdir)/include $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS)
arckill_LDADD    = $(CLILIBS) $(GLIBMM_LIBS)

arcclean_SOURCES  = arcclean.cpp
arcclean_CXXFLAGS = -I$(top_srcdir)/include $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS)
arcclean_LDADD    = $(CLILIBS) $(GLIBMM_LIBS)

arccat_SOURCES  = arccat.cpp
arccat_CXXFLAGS = -I$(top_srcdir)/include $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS)
arccat_LDADD    = $(CLILIBS) $(GLIBMM_LIBS)

arcsync_SOURCES  = arcsync.cpp
arcsync_CXXFLAGS = -I$(top_srcdir)/include $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS)
arcsync_LDADD    = $(CLILIBS) $(GLIBMM_LIBS)

arcresub_SOURCES  = arcresub.cpp
arcresub_CXXFLAGS = -I$(top_srcdir)/include $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS)
arcresub_LDADD    = $(CLILIBS) $(GLIBMM_LIBS)

arcmigrate_SOURCES  = arcmigrate.cpp
arcmigrate_CXXFLAGS = -I$(top_srcdir)/include $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS)
arcmigrate_LDADD    = $(CLILIBS) $(GLIBMM_LIBS)

arcrenew_SOURCES  = arcrenew.cpp
arcrenew_CXXFLAGS = -I$(top_srcdir)/include $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS)
arcrenew_LDADD    = $(CLILIBS) $(GLIBMM_LIBS)

arcresume_SOURCES  = arcresume.cpp
arcresume_CXXFLAGS = -I$(top_srcdir)/include $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS)
arcresume_LDADD    = $(CLILIBS) $(GLIBMM_LIBS)
