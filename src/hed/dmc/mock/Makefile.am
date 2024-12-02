pkglib_LTLIBRARIES = libdmcmock.la
noinst_SCRIPTS = libdmcmock.apd
CLEANFILES=$(noinst_SCRIPTS)
EXTRA_DIST = libdmcmock.apd.in

libdmcmock_la_SOURCES = DataPointMock.cpp DataPointMock.h
libdmcmock_la_CXXFLAGS = -I$(top_srcdir)/include \
	$(LIBXML2_CFLAGS) $(GLIBMM_CFLAGS)
libdmcmock_la_LIBADD = \
	$(top_builddir)/src/hed/libs/data/libarcdata.la \
	$(top_builddir)/src/hed/libs/message/libarcmessage.la \
	$(top_builddir)/src/hed/libs/loader/libarcloader.la \
	$(top_builddir)/src/hed/libs/common/libarccommon.la \
	$(LIBXML2_LIBS) $(GLIBMM_LIBS)
libdmcmock_la_LDFLAGS = -no-undefined -avoid-version -module

libdmcmock.apd: libdmcmock.apd.in libdmcmock.la
	mkdir -p .libs
	cp $(srcdir)/libdmcmock.apd.in .libs/libdmcmock.apd
	cp $(srcdir)/libdmcmock.apd.in libdmcmock.apd

