bin_PROGRAMS = arcstat

CLILIBS = ../../hed/libs/data/libarcdata2.la \	
          ../../hed/libs/loader/libarcloader.la \
          ../../hed/libs/message/libarcmessage.la \
          ../../hed/libs/client/libarclibclient.la\
	  ../../hed/libs/misc/libarcclient.la \
          ../../hed/libs/common/libarccommon.la

arcstat_SOURCES  = arcstat.cpp
arcstat_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
arcstat_LDADD    = $(CLILIBS)

EXTRA_DIST = arccli.cpp
