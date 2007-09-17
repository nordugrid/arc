SUBDIRS = grid-manager
lib_LTLIBRARIES= libarex.la
noinst_PROGRAMS=test

GRIDMANAGER_LIBS = grid-manager/conf/libconf.la grid-manager/jobs/libjobs.la grid-manager/files/libfiles.la grid-manager/run/librun.la grid-manager/jobdesc/libjobdesc.la grid-manager/misc/libmisc.la grid-manager/log/liblog.la grid-manager/url/liburl.la grid-manager/mail/libmail.la grid-manager/jobdesc/jsdl/libnamespace.la

libarex_la_SOURCES=arex.cpp job.cpp PayloadFile.cpp create_activity.cpp get_activity_documents.cpp get_activity_statuses.cpp terminate_activities.cpp get_factory_attributes_document.cpp get.cpp put.cpp arex.h job.h PayloadFile.h
libarex_la_CXXFLAGS=$(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
libarex_la_LIBADD= $(GRIDMANAGER_LIBS) $(top_srcdir)/src/hed/libs/loader/libarcloader.la $(top_srcdir)/src/hed/libs/message/libmessage.la $(top_srcdir)/src/hed/libs/ws-addressing/libwsaddressing.la $(top_srcdir)/src/hed/libs/delegation/libdelegation.la $(top_srcdir)/src/hed/libs/common/libarccommon.la
#libarex_la_LIBADD=grid-manager/conf/libconf.la grid-manager/jobs/libjobs.la grid-manager/files/libfiles.la grid-manager/run/librun.la $(top_srcdir)/src/hed/libs/loader/libarcloader.la $(top_srcdir)/src/hed/libs/message/libmessage.la $(top_srcdir)/src/libs/common/libarccommon.la

test_SOURCES=test.cpp
test_CXXFLAGS=$(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include 
test_LDFLAGS=$(GLIBMM_LIBS) $(LIBXML2_LIBS)
test_LDADD=$(top_srcdir)/src/hed/libs/common/libarccommon.la $(top_srcdir)/src/hed/libs/loader/libarcloader.la $(top_srcdir)/src/hed/libs/message/libmessage.la $(top_srcdir)/src/hed/libs/delegation/libdelegation.la
