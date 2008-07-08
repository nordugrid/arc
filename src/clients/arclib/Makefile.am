bin_PROGRAMS = arcstat arcsub arcget arcclean arckill

man_MANS = arcstat.1 arcsub.1 arcget.1 arcclean.1 arckill.1

CLILIBS = ../../hed/libs/data/libarcdata2.la \
          ../../hed/libs/loader/libarcloader.la \
          ../../hed/libs/message/libarcmessage.la \
          ../../hed/libs/client/libarcclient.la\
          ../../hed/libs/common/libarccommon.la

arcstat_SOURCES  = arcstat.cpp
arcstat_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
arcstat_LDADD    = $(CLILIBS)

arcsub_SOURCES  = arcsub.cpp
arcsub_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
arcsub_LDADD    = $(CLILIBS)

arcget_SOURCES  = arcget.cpp
arcget_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
arcget_LDADD    = $(CLILIBS)

arcclean_SOURCES  = arcclean.cpp
arcclean_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
arcclean_LDADD    = $(CLILIBS)

arckill_SOURCES  = arckill.cpp
arckill_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
arckill_LDADD    = $(CLILIBS)
