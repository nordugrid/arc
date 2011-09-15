lib_LTLIBRARIES = libarcdbxml.la
noinst_PROGRAMS = test

libarcdbxml_ladir = $(pkgincludedir)/dbxml
libarcdbxml_la_HEADERS = XmlDatabase.h XmlContainer.h
libarcdbxml_la_SOURCES = XmlDatabase.cpp XmlContainer.cpp
libarcdbxml_la_CXXFLAGS = -I$(top_srcdir)/include \
	$(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) $(DBCXX_CPPFLAGS)
libarcdbxml_la_LIBADD = \
	$(top_builddir)/src/hed/libs/common/libarccommon.la \
	$(GLIBMM_LIBS) $(LIBXML2_LIBS) $(DBCXX_LIBS)

libarcdbxml_la_LDFLAGS = -version-info 1:0:0

test_SOURCES = test.cpp
test_CXXFLAGS = -I$(top_srcdir)/include \
	$(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) $(DBCXX_CPPFLAGS)
test_LDADD = \
	libarcdbxml.la \
	$(top_builddir)/src/hed/libs/common/libarccommon.la \
	$(GLIBMM_LIBS) $(LIBXML2_LIBS)
