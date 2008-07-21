lib_LTLIBRARIES=libarcdbxml.la
noinst_PROGRAMS=test

libarcdbxml_ladir = $(pkgincludedir)/dbxml
libarcdbxml_la_HEADERS  = XmlDatabase.h XmlContainer.h
libarcdbxml_la_SOURCES  = XmlDatabase.cpp XmlContainer.cpp
libarcdbxml_la_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
libarcdbxml_la_LIBADD   = $(top_srcdir)/src/hed/libs/common/libarccommon.la

libarcdbxml_la_LDFLAGS  = $(GLIBMM_LIBS) $(LIBXML2_LIBS) $(DBCXX_LIBS)

test_SOURCES  = test.cpp
test_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
test_LDFLAGS  = $(GLIBMM_LIBS) $(LIBXML2_LIBS) 
test_LDADD    = libarcdbxml.la
