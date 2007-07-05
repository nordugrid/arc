noinst_LTLIBRARIES = libfiles.la

libfiles_ladir=$(includedir)
libfiles_la_HEADERS=delete.h   info_files.h   info_types.h   info_log.h
libfiles_la_SOURCES=delete.cpp info_files.cpp info_types.cpp info_log.cpp
libfiles_la_CPPFLAGS=$(LIBXML2_CFLAGS) $(GLIBMM_CFLAGS) $(GLOBUS_CFLAGS)
libfiles_la_LIBADD=$(top_srcdir)/src/services/a-rex/grid-manager/jobdesc/libjobdesc.la
