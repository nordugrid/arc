SUBDIRS = schema
pkglib_LTLIBRARIES = libcharon.la
noinst_PROGRAMS = test

exampledir = $(pkgdatadir)/examples/charon
example_DATA = charon_service.xml.example charon_policy_arc.xml.example \
	charon_policy_xacml.xml.example

libcharon_la_SOURCES = charon.cpp charon.h
libcharon_la_CXXFLAGS = -I$(top_srcdir)/include \
	$(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS)
libcharon_la_LIBADD = \
	$(top_builddir)/src/hed/libs/ws/libarcws.la \
	$(top_builddir)/src/hed/libs/security/libarcsecurity.la \
	$(top_builddir)/src/hed/libs/message/libarcmessage.la \
	$(top_builddir)/src/hed/libs/loader/libarcloader.la \
	$(top_builddir)/src/hed/libs/common/libarccommon.la \
	$(GLIBMM_LIBS)
libcharon_la_LDFLAGS = -no-undefined -avoid-version -module

test_SOURCES = test.cpp
test_CXXFLAGS = -I$(top_srcdir)/include $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS)
test_LDADD = \
	$(top_builddir)/src/hed/libs/message/libarcmessage.la \
	$(top_builddir)/src/hed/libs/loader/libarcloader.la \
	$(top_builddir)/src/hed/libs/common/libarccommon.la \
	$(GLIBMM_LIBS) $(LIBXML2_LIBS)

EXTRA_DIST = $(example_DATA)
