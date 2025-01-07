pkglibtestdir = $(pkglibdir)/test
pkglibtest_LTLIBRARIES = libaccTEST.la
noinst_SCRIPTS = libaccTEST.apd
CLEANFILES=$(noinst_SCRIPTS)
EXTRA_DIST = libaccTEST.apd.in

libaccTEST_la_SOURCES = \
	JobControllerPluginTestACC.cpp JobControllerPluginTestACC.h \
	JobDescriptionParserPluginTestACC.cpp JobDescriptionParserPluginTestACC.h \
	SubmitterPluginTestACC.cpp SubmitterPluginTestACC.h \
	BrokerPluginTestACC.h \
	TargetInformationRetrieverPluginTEST.cpp TargetInformationRetrieverPluginTEST.h \
	ServiceEndpointRetrieverPluginTEST.cpp ServiceEndpointRetrieverPluginTEST.h \
	JobListRetrieverPluginTEST.cpp JobListRetrieverPluginTEST.h \
	TestACCPluginDescriptors.cpp
libaccTEST_la_CXXFLAGS = -I$(top_srcdir)/include \
	$(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) $(AM_CXXFLAGS)
libaccTEST_la_LIBADD = \
	$(top_builddir)/src/hed/libs/compute/libarccompute.la \
	$(top_builddir)/src/hed/libs/message/libarcmessage.la \
	$(top_builddir)/src/hed/libs/loader/libarcloader.la \
	$(top_builddir)/src/hed/libs/common/libarccommon.la \
	$(GLIBMM_LIBS) $(LIBXML2_LIBS)
libaccTEST_la_LDFLAGS = -no-undefined -avoid-version -module


libaccTEST.apd: libaccTEST.apd.in libaccTEST.la
	mkdir -p .libs
	cp $(srcdir)/libaccTEST.apd.in .libs/libaccTEST.apd
	cp $(srcdir)/libaccTEST.apd.in libaccTEST.apd

