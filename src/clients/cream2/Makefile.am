bin_PROGRAMS = glitesub glitestat glitekill gliteclean glitedelegate
#doc_DATA = client.xml jsdl_simple.xml jsdl_stage.xml
#EXTRA_DIST = $(doc_DATA)
EXTRA_DIST = openSSLFunctions.cpp
SOURCES = cream_client.cpp cream_client.h
LIBS = $(top_srcdir)/src/hed/libs/common/libarccommon.la $(top_srcdir)/src/hed/libs/loader/libarcloader.la $(top_srcdir)/src/hed/libs/message/libarcmessage.la $(top_srcdir)/src/hed/libs/misc/libarcclient.la $(top_srcdir)/src/hed/libs/security/libarcsecurity.la

glitesub_SOURCES  = $(SOURCES) glitesub.cpp
glitesub_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include 
glitesub_LDFLAGS  = $(GLIBMM_LIBS) $(LIBXML2_LIBS)
glitesub_LDADD    = $(LIBS)

glitestat_SOURCES  = $(SOURCES) glitestat.cpp
glitestat_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include 
glitestat_LDFLAGS  = $(GLIBMM_LIBS) $(LIBXML2_LIBS)
glitestat_LDADD    = $(LIBS)

glitekill_SOURCES  = $(SOURCES) glitekill.cpp
glitekill_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
glitekill_LDFLAGS  = $(GLIBMM_LIBS) $(LIBXML2_LIBS)
glitekill_LDADD    = $(LIBS)

gliteclean_SOURCES  = $(SOURCES) gliteclean.cpp
gliteclean_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
gliteclean_LDFLAGS  = $(GLIBMM_LIBS) $(LIBXML2_LIBS)
gliteclean_LDADD    = $(LIBS)

glitedelegate_SOURCES  = $(SOURCES) glitedelegate.cpp
glitedelegate_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
glitedelegate_LDFLAGS  = $(GLIBMM_LIBS) $(LIBXML2_LIBS)
glitedelegate_LDADD    = $(LIBS)

#man_MANS = apsub.1 apstat.1 apkill.1 apsstat.1 apclean.1
