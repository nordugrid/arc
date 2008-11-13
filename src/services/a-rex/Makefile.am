SUBDIRS = grid-manager ldif config infoproviders lrms
DIST_SUBDIRS = grid-manager ldif config infoproviders lrms
pkglib_LTLIBRARIES = libarex.la
noinst_PROGRAMS=test

initddir=@initddir@
initd_SCRIPTS=a-rex

docdir = @docdir@
doc_DATA = arex_service.xml.example arc_arex.conf arex_service-secure.xml.example arex_minimalistic.xml.example

#GRIDMANAGER_LIBS = grid-manager/conf/libconf.la grid-manager/jobs/libjobs.la grid-manager/files/libfiles.la grid-manager/run/librun.la \
#                   grid-manager/jobdesc/libjobdesc.la grid-manager/misc/libmisc.la grid-manager/log/liblog.la \
#                   grid-manager/mail/libmail.la 
GRIDMANAGER_LIBS = grid-manager/libgridmanager.la

libarex_la_SOURCES  = arex.cpp job.cpp PayloadFile.cpp create_activity.cpp get_activity_documents.cpp get_activity_statuses.cpp terminate_activities.cpp \
                      change_activity_status.cpp get_factory_attributes_document.cpp get.cpp put.cpp information_collector.cpp faults.cpp arex.h job.h PayloadFile.h gacl.cpp gacl.h tools.cpp tools.h
libarex_la_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
# Needs real cleaning in respect to dependencies
libarex_la_LIBADD   = $(top_srcdir)/src/hed/libs/loader/libarcloader.la $(top_srcdir)/src/hed/libs/message/libarcmessage.la $(top_srcdir)/src/hed/libs/security/libarcsecurity.la $(top_srcdir)/src/hed/libs/ws/libarcws.la $(top_srcdir)/src/hed/libs/common/libarccommon.la ldif/libldif.la ./config/libconfig.la $(top_srcdir)/src/hed/libs/data/libarcdata2.la $(top_srcdir)/src/hed/libs/delegation/libdelegation.la $(GRIDMANAGER_LIBS)
libarex_la_LDFLAGS=-no-undefined -avoid-version -module

test_SOURCES  = test.cpp
test_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include 
test_LDFLAGS  = $(GLIBMM_LIBS) $(LIBXML2_LIBS)
test_LDADD    = $(top_srcdir)/src/hed/libs/common/libarccommon.la \
                $(top_srcdir)/src/hed/libs/loader/libarcloader.la \
                $(top_srcdir)/src/hed/libs/message/libarcmessage.la \
                $(top_srcdir)/src/hed/libs/security/libarcsecurity.la \
                $(top_srcdir)/src/hed/libs/delegation/libdelegation.la \
                $(top_srcdir)/src/hed/libs/infosys/libinfosys.la #$(top_srcdir)/src/hed/libs/ws/libarcws.la

sysconf_DATA = arc_arex.conf

EXTRA_DIST = $(doc_DATA)
