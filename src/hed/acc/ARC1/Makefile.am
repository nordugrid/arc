pkglib_LTLIBRARIES = libaccARC1.la
#noinst_LTLIBRARIES = libarexclient.la
noinst_LTLIBRARIES = libaccJLRWSRFBES.la

libaccARC1_la_SOURCES  = SubmitterPluginARC1.cpp SubmitterPluginARC1.h \
	JobListRetrieverPluginARC1.cpp JobListRetrieverPluginARC1.h \
	JobControllerPluginARC1.cpp JobControllerPluginARC1.h \
	AREXClient.cpp AREXClient.h \
	DescriptorsARC1.cpp JobStateARC1.h JobStateARC1.cpp \
	TargetInformationRetrieverPluginWSRFGLUE2.cpp TargetInformationRetrieverPluginWSRFGLUE2.h \
	TargetInformationRetrieverPluginBES.cpp TargetInformationRetrieverPluginBES.h \
	JobControllerPluginBES.cpp JobControllerPluginBES.h \
	JobStateBES.cpp JobStateBES.h
libaccARC1_la_CXXFLAGS = -I$(top_srcdir)/include \
	$(LIBXML2_CFLAGS) $(GLIBMM_CFLAGS)
libaccARC1_la_LIBADD = \
	$(top_builddir)/src/hed/libs/ws/libarcws.la \
	$(top_builddir)/src/hed/libs/client/libarcclient.la \
	$(top_builddir)/src/hed/libs/security/libarcsecurity.la \
	$(top_builddir)/src/hed/libs/delegation/libarcdelegation.la \
	$(top_builddir)/src/hed/libs/data/libarcdata.la \
	$(top_builddir)/src/hed/libs/message/libarcmessage.la\
	$(top_builddir)/src/hed/libs/loader/libarcloader.la \
	$(top_builddir)/src/hed/libs/common/libarccommon.la \
	$(LIBXML2_LIBS) $(GLIBMM_LIBS)
libaccARC1_la_LDFLAGS = -no-undefined -avoid-version -module

libaccJLRWSRFBES_la_SOURCES  = \
	JobListRetrieverPluginWSRFBES.cpp JobListRetrieverPluginWSRFBES.h
libaccJLRWSRFBES_la_CXXFLAGS = -I$(top_srcdir)/include \
	$(LIBXML2_CFLAGS) $(GLIBMM_CFLAGS)
libaccJLRWSRFBES_la_LIBADD = \
	$(top_builddir)/src/hed/libs/ws/libarcws.la \
	$(top_builddir)/src/hed/libs/client/libarcclient.la \
	$(top_builddir)/src/hed/libs/security/libarcsecurity.la \
	$(top_builddir)/src/hed/libs/delegation/libarcdelegation.la \
	$(top_builddir)/src/hed/libs/data/libarcdata.la \
	$(top_builddir)/src/hed/libs/message/libarcmessage.la\
	$(top_builddir)/src/hed/libs/loader/libarcloader.la \
	$(top_builddir)/src/hed/libs/common/libarccommon.la \
	$(LIBXML2_LIBS) $(GLIBMM_LIBS)
libaccJLRWSRFBES_la_LDFLAGS = -no-undefined -avoid-version -module

#libarexclient_la_SOURCES = AREXClient.cpp AREXClient.h
#libarexclient_la_CXXFLAGS = -I$(top_srcdir)/include \
#	$(LIBXML2_CFLAGS) $(GLIBMM_CFLAGS)
#libarexclient_la_LIBADD = \
#	$(top_builddir)/src/hed/libs/ws/libarcws.la \
#	$(top_builddir)/src/hed/libs/client/libarcclient.la \
#	$(top_builddir)/src/hed/libs/security/libarcsecurity.la \
#	$(top_builddir)/src/hed/libs/delegation/libarcdelegation.la \
#	$(top_builddir)/src/hed/libs/message/libarcmessage.la \
#	$(top_builddir)/src/hed/libs/common/libarccommon.la \
#	$(LIBXML2_LIBS) $(GLIBMM_LIBS)

DIST_SUBDIRS = test
SUBDIRS = $(TEST_DIR)
