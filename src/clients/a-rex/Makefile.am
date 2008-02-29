bin_PROGRAMS = apsub apstat apkill apsstat apget apclean
doc_DATA = client.xml jsdl_simple.xml jsdl_stage.xml
EXTRA_DIST = $(doc_DATA)
SOURCES = arex_client.cpp arex_client.h

apsub_SOURCES  = $(SOURCES) apsub.cpp
apsub_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include 
apsub_LDFLAGS  = $(GLIBMM_LIBS) $(LIBXML2_LIBS)
apsub_LDADD    = $(top_srcdir)/src/hed/libs/misc/libarcclient.la

apstat_SOURCES  = $(SOURCES) apstat.cpp
apstat_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include 
apstat_LDFLAGS  = $(GLIBMM_LIBS) $(LIBXML2_LIBS)
apstat_LDADD    = $(top_srcdir)/src/hed/libs/misc/libarcclient.la

apkill_SOURCES  = $(SOURCES) apkill.cpp
apkill_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
apkill_LDFLAGS  = $(GLIBMM_LIBS) $(LIBXML2_LIBS)
apkill_LDADD    = $(top_srcdir)/src/hed/libs/misc/libarcclient.la

apclean_SOURCES  = $(SOURCES) apclean.cpp
apclean_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
apclean_LDFLAGS  = $(GLIBMM_LIBS) $(LIBXML2_LIBS)
apclean_LDADD    = $(top_srcdir)/src/hed/libs/misc/libarcclient.la

apsstat_SOURCES  = $(SOURCES) apsstat.cpp
apsstat_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
apsstat_LDFLAGS  = $(GLIBMM_LIBS) $(LIBXML2_LIBS)
apsstat_LDADD    = $(top_srcdir)/src/hed/libs/misc/libarcclient.la

apget_SOURCES  = $(SOURCES) apget.cpp
apget_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
apget_LDFLAGS  = $(GLIBMM_LIBS) $(LIBXML2_LIBS)
apget_LDADD    = $(top_srcdir)/src/hed/libs/misc/libarcclient.la

man_MANS = apsub.1 apstat.1 apkill.1 apsstat.1 apclean.1
