SUBDIRS = 
noinst_LTLIBRARIES = libldif.la
noinst_PROGRAMS=test
bin_PROGRAMS=ldif2xml

libldif_la_SOURCES=LDIFtoXML.cpp LDIFtoXML.h
libldif_la_CXXFLAGS=$(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
libldif_la_LIBADD= $(top_srcdir)/src/hed/libs/common/libarccommon.la
libldif_la_LDFLAGS=

ldif2xml_SOURCES=main.cpp
ldif2xml_CXXFLAGS=$(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
ldif2xml_LDADD=libldif.la

test_SOURCES=test.cpp
test_CXXFLAGS=$(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
test_LDFLAGS=$(GLIBMM_LIBS) $(LIBXML2_LIBS)
test_LDADD=libldif.la
