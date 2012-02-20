lib_LTLIBRARIES = libarctargetinformationretriever.la
pkglib_LTLIBRARIES = libaccTIR.la
pkglibtestdir = $(pkglibdir)/test
pkglibtest_LTLIBRARIES = libaccTIRTEST.la

libarctargetinformationretriever_ladir = $(pkgincludedir)/client
libarctargetinformationretriever_la_HEADERS = TargetInformationRetriever.h TargetInformationRetrieverPlugin.h
libarctargetinformationretriever_la_SOURCES = TargetInformationRetriever.cpp TargetInformationRetrieverPlugin.cpp
libarctargetinformationretriever_la_CXXFLAGS = -I$(top_srcdir)/include \
	$(LIBXML2_CFLAGS) $(GLIBMM_CFLAGS)
libarctargetinformationretriever_la_LIBADD = \
	$(top_builddir)/src/hed/libs/client/libarcclient.la \
	$(top_builddir)/src/hed/libs/loader/libarcloader.la \
	$(top_builddir)/src/hed/libs/common/libarccommon.la \
	$(LIBXML2_LIBS) $(GLIBMM_LIBS)
libarctargetinformationretriever_la_LDFLAGS = -version-info 1:0:0

libaccTIRTEST_la_SOURCES  = TargetInformationRetrieverPluginTEST.cpp TargetInformationRetrieverPluginTEST.h
libaccTIRTEST_la_CXXFLAGS = -I$(top_srcdir)/include \
	$(LIBXML2_CFLAGS) $(GLIBMM_CFLAGS)
libaccTIRTEST_la_LIBADD = \
	libarctargetinformationretriever.la \
	$(top_builddir)/src/hed/libs/client/libarcclient.la \
	$(top_builddir)/src/hed/libs/loader/libarcloader.la \
	$(top_builddir)/src/hed/libs/common/libarccommon.la \
	$(LIBXML2_LIBS) $(GLIBMM_LIBS)
libaccTIRTEST_la_LDFLAGS = -no-undefined -avoid-version -module

libaccTIR_la_SOURCES  = DescriptorsTIR.cpp \
	TargetInformationRetrieverPluginBES.cpp TargetInformationRetrieverPluginBES.h \
	TargetInformationRetrieverPluginWSRFGLUE2.cpp TargetInformationRetrieverPluginWSRFGLUE2.h \
	TargetInformationRetrieverPluginLDAPNG.cpp TargetInformationRetrieverPluginLDAPNG.h \
	TargetInformationRetrieverPluginLDAPGLUE1.cpp TargetInformationRetrieverPluginLDAPGLUE1.h \
	TargetInformationRetrieverPluginEMIES.cpp TargetInformationRetrieverPluginEMIES.h
libaccTIR_la_CXXFLAGS = -I$(top_srcdir)/include \
	$(LIBXML2_CFLAGS) $(GLIBMM_CFLAGS)
libaccTIR_la_LIBADD = \
	libarctargetinformationretriever.la \
	$(top_builddir)/src/hed/libs/client/libarcclient.la \
	$(top_builddir)/src/hed/libs/data/libarcdata2.la \
	$(top_builddir)/src/hed/libs/loader/libarcloader.la \
	$(top_builddir)/src/hed/libs/message/libarcmessage.la \
	$(top_builddir)/src/hed/libs/common/libarccommon.la \
	$(top_builddir)/src/hed/libs/ws/libarcws.la \
	$(top_builddir)/src/hed/libs/delegation/libarcdelegation.la \
	$(top_builddir)/src/hed/libs/credential/libarccredential.la \
	$(LIBXML2_LIBS) $(GLIBMM_LIBS)
libaccTIR_la_LDFLAGS = -no-undefined -avoid-version -module

DIST_SUBDIRS = test
SUBDIRS = $(TEST_DIR)
