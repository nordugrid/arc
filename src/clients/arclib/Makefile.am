bin_PROGRAMS = arcstat arcsub

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
