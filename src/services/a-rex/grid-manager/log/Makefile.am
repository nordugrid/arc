noinst_LTLIBRARIES = liblog.la

liblog_la_SOURCES=job_log.cpp job_log.h
liblog_la_CXXFLAGS=$(LIBXML2_CFLAGS) $(GLIBMM_CFLAGS) -I$(top_srcdir)/include
liblog_la_LIBADD=$(top_srcdir)/src/libs/common/libarccommon.la
#../files/libfiles.la ../conf/libconf.la ../misc/libmisc.la ../run/librun.la ../jobs/libjobs.la
