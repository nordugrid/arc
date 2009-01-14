lib_LTLIBRARIES = libarcjob.la

libarcjob_ladir = $(pkgincludedir)/data
libarcjob_la_HEADERS=runtimeenvironment.h error.h
libarcjob_la_SOURCES=runtimeenvironment.cpp
libarcjob_la_CXXFLAGS=$(LIBXML2_CFLAGS) $(GLIBMM_CFLAGS) -I$(top_srcdir)/include
libarcjob_la_LDFLAGS=
libarcjob_la_LIBADD=$(top_srcdir)/src/hed/libs/common/libarccommon.la

