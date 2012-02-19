lib_LTLIBRARIES = libarcserviceretriever.la
pkglib_LTLIBRARIES = libaccSER.la

pkglibtestdir = $(pkglibdir)/test
pkglibtest_LTLIBRARIES = libaccSERTEST.la

libarcserviceretriever_ladir = $(pkgincludedir)/client
libarcserviceretriever_la_HEADERS = ServiceEndpointRetriever.h ServiceEndpointRetrieverPlugin.h
libarcserviceretriever_la_SOURCES = ServiceEndpointRetriever.cpp ServiceEndpointRetrieverPlugin.cpp
libarcserviceretriever_la_CXXFLAGS = -I$(top_srcdir)/include \
	$(LIBXML2_CFLAGS) $(GLIBMM_CFLAGS)
libarcserviceretriever_la_LIBADD = \
	$(top_builddir)/src/hed/libs/client/libarcclient.la \
	$(top_builddir)/src/hed/libs/loader/libarcloader.la \
	$(top_builddir)/src/hed/libs/common/libarccommon.la \
	$(LIBXML2_LIBS) $(GLIBMM_LIBS)
libarcserviceretriever_la_LDFLAGS = -version-info 1:0:0

libaccSERTEST_la_SOURCES  = ServiceEndpointRetrieverPluginTEST.cpp ServiceEndpointRetrieverPluginTEST.h
libaccSERTEST_la_CXXFLAGS = -I$(top_srcdir)/include \
	$(LIBXML2_CFLAGS) $(GLIBMM_CFLAGS)
libaccSERTEST_la_LIBADD = \
	libarcserviceretriever.la \
	$(top_builddir)/src/hed/libs/client/libarcclient.la \
	$(top_builddir)/src/hed/libs/loader/libarcloader.la \
	$(top_builddir)/src/hed/libs/common/libarccommon.la \
	$(LIBXML2_LIBS) $(GLIBMM_LIBS)
libaccSERTEST_la_LDFLAGS = -no-undefined -avoid-version -module

libaccSER_la_SOURCES  = DescriptorsSER.cpp \
	ServiceEndpointRetrieverPluginEGIIS.cpp ServiceEndpointRetrieverPluginEGIIS.h \
	ServiceEndpointRetrieverPluginEMIR.cpp ServiceEndpointRetrieverPluginEMIR.h
libaccSER_la_CXXFLAGS = -I$(top_srcdir)/include \
	$(LIBXML2_CFLAGS) $(GLIBMM_CFLAGS)
libaccSER_la_LIBADD = \
	libarcserviceretriever.la \
	$(top_builddir)/src/hed/libs/client/libarcclient.la \
	$(top_builddir)/src/hed/libs/data/libarcdata2.la \
	$(top_builddir)/src/hed/libs/loader/libarcloader.la \
	$(top_builddir)/src/hed/libs/message/libarcmessage.la \
	$(top_builddir)/src/hed/libs/common/libarccommon.la \
	$(LIBXML2_LIBS) $(GLIBMM_LIBS)
libaccSER_la_LDFLAGS = -no-undefined -avoid-version -module

DIST_SUBDIRS = test
SUBDIRS = $(TEST_DIR)
