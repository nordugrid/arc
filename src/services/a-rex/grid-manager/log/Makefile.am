noinst_LTLIBRARIES = liblog.la

liblog_la_SOURCES = job_log.cpp job_log.h
liblog_la_CXXFLAGS = -I$(top_srcdir)/include $(LIBXML2_CFLAGS) $(GLIBMM_CFLAGS)
liblog_la_LIBADD = $(top_builddir)/src/hed/libs/common/libarccommon.la
