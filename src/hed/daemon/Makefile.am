sbin_PROGRAMS = arcserver
SOURCES=main.cpp options.cpp options.h daemon.cpp daemon.h

arcserver_SOURCES  = $(SOURCES)
arcserver_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
arcserver_LDFLAGS  = $(GLIBMM_LIBS) $(LIBXML2_LIBS)
arcserver_LDADD    = $(top_srcdir)/src/hed/libs/common/libarccommon.la $(top_srcdir)/src/hed/libs/loader/libarcloader.la $(top_srcdir)/src/hed/libs/message/libarcmessage.la

man_MANS = arcserver.8
