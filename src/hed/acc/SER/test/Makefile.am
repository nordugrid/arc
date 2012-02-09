TESTS = ServiceEndpointRetrieverTest
check_PROGRAMS = $(TESTS)

TESTS_ENVIRONMENT = env ARC_PLUGIN_PATH=$(top_builddir)/src/hed/acc/SER/.libs

ServiceEndpointRetrieverTest_SOURCES = $(top_srcdir)/src/Test.cpp \
	ServiceEndpointRetrieverTest.cpp
ServiceEndpointRetrieverTest_CXXFLAGS = -I$(top_srcdir)/include \
	$(CPPUNIT_CFLAGS) $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS)
ServiceEndpointRetrieverTest_LDADD = \
	../libarcserviceretriever.la \
	$(top_builddir)/src/hed/libs/client/libarcclient.la \
	$(top_builddir)/src/hed/libs/loader/libarcloader.la \
	$(top_builddir)/src/hed/libs/common/libarccommon.la \
	$(CPPUNIT_LIBS) $(GLIBMM_LIBS) $(LIBXML2_LIBS)
