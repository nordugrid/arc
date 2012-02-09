pkglib_LTLIBRARIES = libaccSERTEST.la
lib_LTLIBRARIES = libarcserviceretriever.la

libarcserviceretriever_ladir = $(pkgincludedir)/client
libarcserviceretriever_la_HEADERS = ServiceEndpointRetriever.h
libarcserviceretriever_la_SOURCES = ServiceEndpointRetriever.cpp
libarcserviceretriever_la_CXXFLAGS = -I$(top_srcdir)/include \
	$(CFLAGS_WITH_XMLSEC) $(LIBXML2_CFLAGS) $(GLIBMM_CFLAGS)
libarcserviceretriever_la_LIBADD = \
	$(top_builddir)/src/hed/libs/client/libarcclient.la \
	$(top_builddir)/src/hed/libs/loader/libarcloader.la \
	$(top_builddir)/src/hed/libs/common/libarccommon.la \
	$(LIBS_WITH_XMLSEC) $(LIBXML2_LIBS) $(GLIBMM_LIBS)
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

DIST_SUBDIRS = test
SUBDIRS = $(TEST_DIR)
