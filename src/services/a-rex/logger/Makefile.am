SUBDIRS = 
DIST_SUBDIRS = 
pkglib_LTLIBRARIES = 
noinst_PROGRAMS=logger

logger_SOURCES  = JobRecord.cpp JobRecord.h client.cpp client.h logger.cpp
logger_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
logger_LDADD   = ../grid-manager/conf/libconf.la ../grid-manager/misc/libmisc.la $(top_srcdir)/src/hed/libs/common/libarccommon.la $(top_srcdir)/src/hed/acc/ARC1/libarexclient.la
logger_LDFLAGS  =
