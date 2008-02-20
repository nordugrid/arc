bin_PROGRAMS = apsub apstat apkill apsstat
bin_SCRIPTS = arex_client.xml
EXTRA_DIST = $(bin_SCRIPTS)
SOURCES = arex_client.cpp arex_client.h

apsub_SOURCES  = $(SOURCES) apsub.cpp
apsub_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include 
apsub_LDFLAGS  = $(GLIBMM_LIBS) $(LIBXML2_LIBS)
apsub_LDADD    = $(top_srcdir)/src/hed/libs/common/libarccommon.la $(top_srcdir)/src/hed/libs/loader/libarcloader.la $(top_srcdir)/src/hed/libs/message/libarcmessage.la

apstat_SOURCES  = $(SOURCES) apstat.cpp
apstat_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include 
apstat_LDFLAGS  = $(GLIBMM_LIBS) $(LIBXML2_LIBS)
apstat_LDADD    = $(top_srcdir)/src/hed/libs/common/libarccommon.la $(top_srcdir)/src/hed/libs/loader/libarcloader.la $(top_srcdir)/src/hed/libs/message/libarcmessage.la

apkill_SOURCES  = $(SOURCES) apkill.cpp
apkill_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
apkill_LDFLAGS  = $(GLIBMM_LIBS) $(LIBXML2_LIBS)
apkill_LDADD    = $(top_srcdir)/src/hed/libs/common/libarccommon.la $(top_srcdir)/src/hed/libs/loader/libarcloader.la $(top_srcdir)/src/hed/libs/message/libarcmessage.la

apsstat_SOURCES  = $(SOURCES) apsstat.cpp
apsstat_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
apsstat_LDFLAGS  = $(GLIBMM_LIBS) $(LIBXML2_LIBS)
apsstat_LDADD    = $(top_srcdir)/src/hed/libs/common/libarccommon.la $(top_srcdir)/src/hed/libs/loader/libarcloader.la $(top_srcdir)/src/hed/libs/message/libarcmessage.la
