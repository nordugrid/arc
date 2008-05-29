bin_PROGRAMS = apinfo

apinfo_SOURCES  = apinfo.cpp
apinfo_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include 
apinfo_LDFLAGS  = $(GLIBMM_LIBS) $(LIBXML2_LIBS)
apinfo_LDADD    = $(top_srcdir)/src/hed/libs/common/libarccommon.la $(top_srcdir)/src/hed/libs/loader/libarcloader.la \
                  $(top_srcdir)/src/hed/libs/message/libarcmessage.la  $(top_srcdir)/src/hed/libs/client/libarcclient.la   \
                  $(top_srcdir)/src/hed/libs/infosys/libinfosys.la

man_MANS = apinfo.1