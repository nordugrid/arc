bin_PROGRAMS = arcwsrf

man_MANS = arcwsrf.1

arcwsrf_SOURCES  = arcwsrf.cpp
arcwsrf_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
arcwsrf_LDADD    = ../../hed/libs/ws/libarcws.la ../../hed/libs/client/libarcclient.la

sysconf_DATA =
doc_DATA =
EXTRA_DIST = $(doc_DATA) $(sysconf_DATA)
