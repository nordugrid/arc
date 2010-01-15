bin_PROGRAMS = wsdl2hed arcplugin

man_MANS = wsdl2hed.1 arcplugin.1

wsdl2hed_SOURCES  = wsdl2hed.cpp schemaconv.cpp schemaconv.h
wsdl2hed_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
wsdl2hed_LDFLAGS  = $(GLIBMM_LIBS) $(LIBXML2_LIBS) $(EXTRA_LIBS)
wsdl2hed_LDADD    = $(top_srcdir)/src/hed/libs/common/libarccommon.la

arcplugin_SOURCES  = arcplugin.cpp
arcplugin_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
arcplugin_LDFLAGS  = $(GLIBMM_LIBS) $(LIBXML2_LIBS) $(EXTRA_LIBS)
arcplugin_LDADD    = $(top_srcdir)/src/hed/libs/common/libarccommon.la
