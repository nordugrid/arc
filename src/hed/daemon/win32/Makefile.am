noinst_LTLIBRARIES = libdaemon.la

libdaemon_la_SOURCES = main_win32.cpp
libdaemon_la_CXXFLAGS = -I$(top_srcdir)/include \
	$(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS)
libdaemon_la_LIBADD = \
	$(top_builddir)/src/hed/libs/common/libarccommon.la \
	$(LIBXML2_LIBS)
