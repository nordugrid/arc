bin_PROGRAMS = arcwsrf

man_MANS = arcwsrf.1

arcwsrf_SOURCES = arcwsrf.cpp
arcwsrf_CXXFLAGS = -I$(top_srcdir)/include $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS)
arcwsrf_LDADD = \
	$(top_builddir)/src/hed/libs/ws/libarcws.la \
	$(top_builddir)/src/hed/libs/client/libarcclient.la \
	$(top_builddir)/src/hed/libs/message/libarcmessage.la \
	$(top_builddir)/src/hed/libs/common/libarccommon.la \
	$(GLIBMM_LIBS)

sysconf_DATA =
doc_DATA =
EXTRA_DIST = $(doc_DATA) $(sysconf_DATA)
