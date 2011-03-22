noinst_LTLIBRARIES = libldif.la
noinst_PROGRAMS = test ldif2xml

libldif_la_SOURCES = LDIFtoXML.cpp LDIFtoXML.h
libldif_la_CXXFLAGS = -I$(top_srcdir)/include \
	$(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS)
libldif_la_LIBADD = $(top_srcdir)/src/hed/libs/common/libarccommon.la

ldif2xml_SOURCES = main.cpp
ldif2xml_CXXFLAGS = -I$(top_srcdir)/include $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS)
ldif2xml_LDADD = libldif.la

test_SOURCES = test.cpp
test_CXXFLAGS = -I$(top_srcdir)/include $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS)
test_LDADD = libldif.la $(GLIBMM_LIBS) $(LIBXML2_LIBS)
