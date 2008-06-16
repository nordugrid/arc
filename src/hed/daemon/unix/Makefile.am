noinst_LTLIBRARIES= libdaemon.la

libdaemon_la_SOURCES  = daemon.cpp main_unix.cpp daemon.h
libdaemon_la_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
libdaemon_la_LIBADD   = $(top_srcdir)/src/hed/libs/common/libarccommon.la
libdaemon_la_LDFLAGS  = $(LIBXML2_LIBS)

