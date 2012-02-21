TESTS = TargetInformationRetrieverTest
check_PROGRAMS = $(TESTS)

TESTS_ENVIRONMENT = env ARC_PLUGIN_PATH=$(top_builddir)/src/hed/acc/TIR/.libs

TargetInformationRetrieverTest_SOURCES = $(top_srcdir)/src/Test.cpp \
	TargetInformationRetrieverTest.cpp
TargetInformationRetrieverTest_CXXFLAGS = -I$(top_srcdir)/include \
	$(CPPUNIT_CFLAGS) $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS)
TargetInformationRetrieverTest_LDADD = \
	../libarctargetinformationretriever.la \
	$(top_builddir)/src/hed/libs/ws/libarcws.la \
	$(top_builddir)/src/hed/libs/client/libarcclient.la \
	$(top_builddir)/src/hed/libs/loader/libarcloader.la \
	$(top_builddir)/src/hed/libs/common/libarccommon.la \
	$(CPPUNIT_LIBS) $(GLIBMM_LIBS) $(LIBXML2_LIBS)
