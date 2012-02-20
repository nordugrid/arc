lib_LTLIBRARIES = libarctargetinformationretriever.la

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

DIST_SUBDIRS = test
SUBDIRS = $(TEST_DIR)
