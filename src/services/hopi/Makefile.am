pkglib_LTLIBRARIES = libhopi.la

exampledir = $(pkgdatadir)/examples/hopi
example_DATA = hopi_service.xml.example

libhopi_la_SOURCES = hopi.cpp PayloadFile.cpp hopi.h PayloadFile.h
libhopi_la_CXXFLAGS = -I$(top_srcdir)/include \
	$(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS)
libhopi_la_LIBADD = \
	$(top_srcdir)/src/hed/libs/ws/libarcws.la \
	$(top_srcdir)/src/hed/libs/message/libarcmessage.la \
	$(top_srcdir)/src/hed/libs/loader/libarcloader.la \
	$(top_srcdir)/src/hed/libs/common/libarccommon.la \
	$(GLIBMM_LIBS)
libhopi_la_LDFLAGS = -no-undefined -avoid-version -module

EXTRA_DIST = $(example_DATA)
