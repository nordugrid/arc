bin_PROGRAMS = arcsub arcget arcstat arcinfo arckill arcclean arccat arcsync arcresub \
	arcmigrate arcresume arcrenew

man_MANS = arcsub.1 arcget.1 arcstat.1 arcinfo.1 arckill.1 arcclean.1 arccat.1 \
	arcresub.1 arcmigrate.1 arcsync.1 arcresume.1 arcrenew.1

CLILIBS = ../../hed/libs/data/libarcdata2.la \
          ../../hed/libs/loader/libarcloader.la \
          ../../hed/libs/message/libarcmessage.la \
          ../../hed/libs/client/libarcclient.la\
          ../../hed/libs/common/libarccommon.la

arcsub_SOURCES  = arcsub.cpp
arcsub_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
arcsub_LDADD    = $(CLILIBS)

arcget_SOURCES  = arcget.cpp
arcget_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
arcget_LDADD    = $(CLILIBS)

arcstat_SOURCES  = arcstat.cpp
arcstat_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
arcstat_LDADD    = $(CLILIBS)

arcinfo_SOURCES  = arcinfo.cpp
arcinfo_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
arcinfo_LDADD    = $(CLILIBS)

arckill_SOURCES  = arckill.cpp
arckill_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
arckill_LDADD    = $(CLILIBS)

arcclean_SOURCES  = arcclean.cpp
arcclean_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
arcclean_LDADD    = $(CLILIBS)

arccat_SOURCES  = arccat.cpp
arccat_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
arccat_LDADD    = $(CLILIBS)

arcsync_SOURCES  = arcsync.cpp
arcsync_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
arcsync_LDADD    = $(CLILIBS)

arcresub_SOURCES  = arcresub.cpp
arcresub_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
arcresub_LDADD    = $(CLILIBS)

arcmigrate_SOURCES  = arcmigrate.cpp
arcmigrate_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
arcmigrate_LDADD    = $(CLILIBS)

arcrenew_SOURCES  = arcrenew.cpp
arcrenew_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
arcrenew_LDADD    = $(CLILIBS)

arcresume_SOURCES  = arcresume.cpp
arcresume_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
arcresume_LDADD    = $(CLILIBS)
