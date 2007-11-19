bin_PROGRAMS = wsdl2hed
SOURCES = wsdl2hed.cpp

wsdl2hed_SOURCES  = $(SOURCES)
wsdl2hed_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
wsdl2hed_LDFLAGS  = $(GLIBMM_LIBS) $(LIBXML2_LIBS) $(EXTRA_LIBS)
wsdl2hed_LDADD    = $(top_srcdir)/src/hed/libs/common/libarccommon.la

