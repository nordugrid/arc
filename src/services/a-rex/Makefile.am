SUBDIRS = grid-manager ldif config infoproviders lrms
DIST_SUBDIRS = grid-manager ldif config infoproviders lrms
pkglib_LTLIBRARIES = libarex.la
noinst_PROGRAMS=test

docdir = @docdir@
doc_DATA = arex.xml arc_arex.conf jsdl_arex.xml

#GRIDMANAGER_LIBS = grid-manager/conf/libconf.la grid-manager/jobs/libjobs.la grid-manager/files/libfiles.la grid-manager/run/librun.la \
#                   grid-manager/jobdesc/libjobdesc.la grid-manager/misc/libmisc.la grid-manager/log/liblog.la \
#                   grid-manager/mail/libmail.la 
GRIDMANAGER_LIBS = grid-manager/libgridmanager.la

libarex_la_SOURCES  = arex.cpp job.cpp PayloadFile.cpp create_activity.cpp get_activity_documents.cpp get_activity_statuses.cpp terminate_activities.cpp \
                      get_factory_attributes_document.cpp get.cpp put.cpp information_collector.cpp arex.h job.h PayloadFile.h gacl.cpp gacl.h
libarex_la_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
# Needs real cleaning in respect to dependencies
libarex_la_LIBADD   = $(GRIDMANAGER_LIBS) $(top_srcdir)/src/hed/libs/loader/libarcloader.la $(top_srcdir)/src/hed/libs/message/libarcmessage.la $(top_srcdir)/src/hed/libs/security/libarcsecurity.la $(top_srcdir)/src/hed/libs/ws/libarcws.la $(top_srcdir)/src/hed/libs/common/libarccommon.la ldif/libldif.la ./config/libconfig.la $(top_srcdir)/src/hed/libs/data/libarcdata2.la
libarex_la_LDFLAGS=-no-undefined -avoid-version -module

test_SOURCES  = test.cpp
test_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include 
test_LDFLAGS  = $(GLIBMM_LIBS) $(LIBXML2_LIBS)
test_LDADD    = $(top_srcdir)/src/hed/libs/common/libarccommon.la $(top_srcdir)/src/hed/libs/loader/libarcloader.la $(top_srcdir)/src/hed/libs/message/libarcmessage.la \
                $(top_srcdir)/src/hed/libs/security/libarcsecurity.la $(top_srcdir)/src/hed/libs/ws/libarcws.la

EXTRA_DIST = $(doc_DATA)
