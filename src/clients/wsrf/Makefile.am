bin_PROGRAMS = arcwsrf

man_MANS = arcwsrf.1

arcwsrf_SOURCES = arcwsrf.cpp
arcwsrf_CXXFLAGS = -I$(top_srcdir)/include \
	$(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) $(AM_CXXFLAGS)
arcwsrf_LDADD = \
	$(top_builddir)/src/hed/libs/infosys/libarcinfosys.la \
	$(top_builddir)/src/hed/libs/communication/libarccommunication.la \
	$(top_builddir)/src/hed/libs/message/libarcmessage.la \
	$(top_builddir)/src/hed/libs/common/libarccommon.la \
	$(GLIBMM_LIBS)

sysconf_DATA =
doc_DATA =
EXTRA_DIST = $(doc_DATA) $(sysconf_DATA)
