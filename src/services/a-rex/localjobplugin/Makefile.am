pkglib_LTLIBRARIES = libaccLOCAL.la

libaccLOCAL_la_SOURCES  = LOCALClient.cpp LOCALClient.h \
	JobStateLOCAL.cpp JobStateLOCAL.h \
	SubmitterPluginLOCAL.cpp SubmitterPluginLOCAL.h \
	JobControllerPluginLOCAL.cpp JobControllerPluginLOCAL.h \
	JobListRetrieverPluginLOCAL.cpp JobListRetrieverPluginLOCAL.h \
	TargetInformationRetrieverPluginLOCAL.cpp TargetInformationRetrieverPluginLOCAL.h \
	DescriptorsLOCAL.cpp \
	../job.cpp ../tools.cpp
libaccLOCAL_la_CXXFLAGS = -I$(top_srcdir)/include \
	$(LIBXML2_CFLAGS) $(GLIBMM_CFLAGS) $(AM_CXXFLAGS)


libaccLOCAL_la_LIBADD = \
	$(top_builddir)/src/hed/libs/common/libarccommon.la \
	$(top_builddir)/src/hed/libs/credential/libarccredential.la \
	$(top_builddir)/src/hed/libs/compute/libarccompute.la \
	$(top_builddir)/src/hed/libs/security/libarcsecurity.la \
	../grid-manager/conf/libconf.la \
	../grid-manager/jobs/libjobs.la \
	../grid-manager/files/libfiles.la \
	../grid-manager/log/liblog.la \
	../grid-manager/run/librun.la \
	../grid-manager/mail/libmail.la \
	../delegation/libdelegation.la


#	$(top_builddir)/src/hed/libs/delegation/libarcdelegation.la \
#	$(top_builddir)/src/hed/libs/delegation/libarcdelegation.la \
#	$(top_builddir)/src/hed/libs/compute/libarccompute.la \
#	$(top_builddir)/src/hed/libs/communication/libarccommunication.la \
#	$(top_builddir)/src/hed/libs/message/libarcmessage.la \
#	$(top_builddir)/src/libs/data-staging/libarcdatastaging.la \
#	$(top_builddir)/src/services/a-rex/grid-manager/libgridmanager.la \
#	$(top_builddir)/src/services/a-rex/delegation/libdelegation.la \
#	$(LIBXML2_LIBS) $(GLIBMM_LIBS) $(GLOBUS_JOBPLUGIN_LIBS)

libaccLOCAL_la_LDFLAGS = -no-undefined -avoid-version -module

