pkglib_LTLIBRARIES = libaccSER.la

libaccSER_la_SOURCES  = DescriptorsSER.cpp \
	ServiceEndpointRetrieverPluginEMIR.cpp ServiceEndpointRetrieverPluginEMIR.h
libaccSER_la_CXXFLAGS = -I$(top_srcdir)/include \
	$(LIBXML2_CFLAGS) $(GLIBMM_CFLAGS)
libaccSER_la_LIBADD = \
	$(top_builddir)/src/hed/libs/client/libarcclient.la \
	$(top_builddir)/src/hed/libs/data/libarcdata.la \
	$(top_builddir)/src/hed/libs/loader/libarcloader.la \
	$(top_builddir)/src/hed/libs/message/libarcmessage.la \
	$(top_builddir)/src/hed/libs/common/libarccommon.la \
	$(LIBXML2_LIBS) $(GLIBMM_LIBS)
libaccSER_la_LDFLAGS = -no-undefined -avoid-version -module
