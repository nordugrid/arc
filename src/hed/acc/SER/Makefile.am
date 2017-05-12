pkglib_LTLIBRARIES = libaccSER.la

if LDNS_ENABLED
ARCHERY_SOURCES = ServiceEndpointRetrieverPluginARCHERY.cpp ServiceEndpointRetrieverPluginARCHERY.h
else
ARCHERY_SOURCES = 
endif

libaccSER_la_SOURCES  = DescriptorsSER.cpp \
	ServiceEndpointRetrieverPluginEMIR.cpp ServiceEndpointRetrieverPluginEMIR.h \
	$(ARCHERY_SOURCES)
libaccSER_la_CXXFLAGS = -I$(top_srcdir)/include \
	$(LIBXML2_CFLAGS) $(GLIBMM_CFLAGS) $(AM_CXXFLAGS) $(LDNS_CFLAGS)
libaccSER_la_LIBADD = \
	$(top_builddir)/src/hed/libs/compute/libarccompute.la \
	$(top_builddir)/src/hed/libs/communication/libarccommunication.la \
	$(top_builddir)/src/hed/libs/data/libarcdata.la \
	$(top_builddir)/src/hed/libs/loader/libarcloader.la \
	$(top_builddir)/src/hed/libs/message/libarcmessage.la \
	$(top_builddir)/src/hed/libs/common/libarccommon.la \
	$(LIBXML2_LIBS) $(GLIBMM_LIBS) $(LDNS_LIBS)
libaccSER_la_LDFLAGS = -no-undefined -avoid-version -module
