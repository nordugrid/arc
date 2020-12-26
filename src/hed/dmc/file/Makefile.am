pkglib_LTLIBRARIES = libdmcfile.la
noinst_SCRIPTS = libdmcfile.apd
CLEANFILES=$(noinst_SCRIPTS)
EXTRA_DIST = libdmcfile.apd.in

libdmcfile_la_SOURCES = DataPointFile.cpp DataPointFile.h
libdmcfile_la_CXXFLAGS = -I$(top_srcdir)/include \
	$(LIBXML2_CFLAGS) $(GLIBMM_CFLAGS) $(AM_CXXFLAGS)
libdmcfile_la_LIBADD = \
	$(top_builddir)/src/hed/libs/data/libarcdata.la \
	$(top_builddir)/src/hed/libs/message/libarcmessage.la \
	$(top_builddir)/src/hed/libs/loader/libarcloader.la \
	$(top_builddir)/src/hed/libs/common/libarccommon.la \
	$(LIBXML2_LIBS) $(GLIBMM_LIBS)
libdmcfile_la_LDFLAGS = -no-undefined -avoid-version -module


libdmcfile.apd: libdmcfile.apd.in libdmcfile.la
	mkdir -p .libs
	cp libdmcfile.apd.in .libs/libdmcfile.apd
	cp libdmcfile.apd.in libdmcfile.apd


