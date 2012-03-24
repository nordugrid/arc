pkglibtestdir = $(pkglibdir)/test
pkglibtest_LTLIBRARIES = libaccTEST.la

libaccTEST_la_SOURCES = TestACCPluginDescriptors.cpp \
	JobControllerTestACC.cpp JobControllerTestACC.h \
	JobDescriptionParserTestACC.cpp JobDescriptionParserTestACC.h \
	SubmitterTestACC.cpp SubmitterTestACC.h \
	BrokerPluginTestACC.h \
	TargetInformationRetrieverPluginTEST.cpp TargetInformationRetrieverPluginTEST.h \
	ServiceEndpointRetrieverPluginTEST.cpp ServiceEndpointRetrieverPluginTEST.h \
	JobListRetrieverPluginTEST.cpp JobListRetrieverPluginTEST.h
libaccTEST_la_CXXFLAGS = -I$(top_srcdir)/include \
	$(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS)
libaccTEST_la_LIBADD = \
	$(top_builddir)/src/hed/libs/client/libarcclient.la \
	$(top_builddir)/src/hed/libs/message/libarcmessage.la \
	$(top_builddir)/src/hed/libs/loader/libarcloader.la \
	$(top_builddir)/src/hed/libs/common/libarccommon.la \
	$(GLIBMM_LIBS) $(LIBXML2_LIBS)
libaccTEST_la_LDFLAGS = -no-undefined -avoid-version -module
