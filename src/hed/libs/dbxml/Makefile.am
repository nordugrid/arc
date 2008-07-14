noinst_LTLIBRARIES=libdbxml.la
noinst_PROGRAMS=test

libdbxml_ladir = $(pkgincludedir)/dbxml
libdbxml_la_HEADERS  = XmlDatabase.h XmlContainer.h
libdbxml_la_SOURCES  = XmlDatabase.cpp XmlContainer.cpp
libdbxml_la_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
libdbxml_la_LIBADD   = $(top_srcdir)/src/hed/libs/common/libarccommon.la

libdbxml_la_LDFLAGS  = $(LIBXML2_LIBS) $(DBCXX_LIBS)

test_SOURCES  = test.cpp
test_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
test_LDFLAGS  = $(GLIBMM_LIBS) $(LIBXML2_LIBS) 
test_LDADD    = libdbxml.la
