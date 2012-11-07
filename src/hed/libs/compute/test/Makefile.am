TESTS = SoftwareTest JobTest JobStateTest BrokerTest JobDescriptionTest \
	JobControllerPluginTest JobDescriptionParserTest SubmitterTest SubmitterPluginTest \
	JobSupervisorTest TargetInformationRetrieverTest \
	ServiceEndpointRetrieverTest JobListRetrieverTest \
	ExecutionTargetTest ComputingServiceUniqTest

check_PROGRAMS = $(TESTS)

TESTS_ENVIRONMENT = env ARC_PLUGIN_PATH=$(top_builddir)/src/hed/acc/TEST/.libs

SoftwareTest_SOURCES = $(top_srcdir)/src/Test.cpp SoftwareTest.cpp
SoftwareTest_CXXFLAGS = -I$(top_srcdir)/include \
	$(CPPUNIT_CFLAGS) $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS)
SoftwareTest_LDADD = \
	$(top_builddir)/src/hed/libs/compute/libarccompute.la \
	$(top_builddir)/src/hed/libs/common/libarccommon.la \
	$(CPPUNIT_LIBS) $(GLIBMM_LIBS) $(LIBXML2_LIBS)

JobTest_SOURCES = $(top_srcdir)/src/Test.cpp JobTest.cpp
JobTest_CXXFLAGS = -I$(top_srcdir)/include \
	$(CPPUNIT_CFLAGS) $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS)
JobTest_LDADD = \
	$(top_builddir)/src/hed/libs/compute/libarccompute.la \
	$(top_builddir)/src/hed/libs/common/libarccommon.la \
	$(CPPUNIT_LIBS) $(GLIBMM_LIBS) $(LIBXML2_LIBS)

JobDescriptionTest_SOURCES = $(top_srcdir)/src/Test.cpp JobDescriptionTest.cpp
JobDescriptionTest_CXXFLAGS = -I$(top_srcdir)/include \
	$(CPPUNIT_CFLAGS) $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS)
JobDescriptionTest_LDADD = \
	$(top_builddir)/src/hed/libs/compute/libarccompute.la \
	$(top_builddir)/src/hed/libs/common/libarccommon.la \
	$(CPPUNIT_LIBS) $(GLIBMM_LIBS) $(LIBXML2_LIBS)

JobStateTest_SOURCES = $(top_srcdir)/src/Test.cpp JobStateTest.cpp
JobStateTest_CXXFLAGS = -I$(top_srcdir)/include \
	$(CPPUNIT_CFLAGS) $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS)
JobStateTest_LDADD = \
	$(top_builddir)/src/hed/libs/compute/libarccompute.la \
	$(top_builddir)/src/hed/libs/common/libarccommon.la \
	$(CPPUNIT_LIBS) $(GLIBMM_LIBS) $(LIBXML2_LIBS)

BrokerTest_SOURCES = $(top_srcdir)/src/Test.cpp BrokerTest.cpp
BrokerTest_CXXFLAGS = -I$(top_srcdir)/include \
	$(CPPUNIT_CFLAGS) $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS)
BrokerTest_LDADD = \
	$(top_builddir)/src/hed/libs/compute/libarccompute.la \
	$(top_builddir)/src/hed/libs/loader/libarcloader.la \
	$(top_builddir)/src/hed/libs/common/libarccommon.la \
	$(CPPUNIT_LIBS) $(GLIBMM_LIBS) $(LIBXML2_LIBS)

JobControllerPluginTest_SOURCES = $(top_srcdir)/src/Test.cpp JobControllerPluginTest.cpp
JobControllerPluginTest_CXXFLAGS = -I$(top_srcdir)/include \
	$(CPPUNIT_CFLAGS) $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS)
JobControllerPluginTest_LDADD = \
	$(top_builddir)/src/hed/libs/compute/libarccompute.la \
	$(top_builddir)/src/hed/libs/common/libarccommon.la \
	$(CPPUNIT_LIBS) $(GLIBMM_LIBS) $(LIBXML2_LIBS)

JobSupervisorTest_SOURCES = $(top_srcdir)/src/Test.cpp \
	JobSupervisorTest.cpp
JobSupervisorTest_CXXFLAGS = -I$(top_srcdir)/include\
	$(CPPUNIT_CFLAGS) $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS)
JobSupervisorTest_LDADD = \
	$(top_builddir)/src/hed/libs/compute/libarccompute.la \
	$(top_builddir)/src/hed/libs/common/libarccommon.la \
	$(CPPUNIT_LIBS) $(GLIBMM_LIBS) $(LIBXML2_LIBS)

JobDescriptionParserTest_SOURCES = $(top_srcdir)/src/Test.cpp \
	JobDescriptionParserTest.cpp
JobDescriptionParserTest_CXXFLAGS = -I$(top_srcdir)/include \
	$(CPPUNIT_CFLAGS) $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS)
JobDescriptionParserTest_LDADD = \
	$(top_builddir)/src/hed/libs/compute/libarccompute.la \
	$(top_builddir)/src/hed/libs/common/libarccommon.la \
	$(CPPUNIT_LIBS) $(GLIBMM_LIBS) $(LIBXML2_LIBS)

SubmitterTest_SOURCES = $(top_srcdir)/src/Test.cpp SubmitterTest.cpp
SubmitterTest_CXXFLAGS = -I$(top_srcdir)/include \
	$(CPPUNIT_CFLAGS) $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS)
SubmitterTest_LDADD = \
	$(top_builddir)/src/hed/libs/compute/libarccompute.la \
	$(top_builddir)/src/hed/libs/common/libarccommon.la \
	$(CPPUNIT_LIBS) $(GLIBMM_LIBS) $(LIBXML2_LIBS)


SubmitterPluginTest_SOURCES = $(top_srcdir)/src/Test.cpp SubmitterPluginTest.cpp
SubmitterPluginTest_CXXFLAGS = -I$(top_srcdir)/include \
	$(CPPUNIT_CFLAGS) $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS)
SubmitterPluginTest_LDADD = \
	$(top_builddir)/src/hed/libs/compute/libarccompute.la \
	$(top_builddir)/src/hed/libs/common/libarccommon.la \
	$(CPPUNIT_LIBS) $(GLIBMM_LIBS) $(LIBXML2_LIBS)

ServiceEndpointRetrieverTest_SOURCES = $(top_srcdir)/src/Test.cpp \
	ServiceEndpointRetrieverTest.cpp
ServiceEndpointRetrieverTest_CXXFLAGS = -I$(top_srcdir)/include \
	$(CPPUNIT_CFLAGS) $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS)
ServiceEndpointRetrieverTest_LDADD = \
	$(top_builddir)/src/hed/libs/compute/libarccompute.la \
	$(top_builddir)/src/hed/libs/common/libarccommon.la \
	$(top_builddir)/src/hed/libs/loader/libarcloader.la \
	$(CPPUNIT_LIBS) $(GLIBMM_LIBS) $(LIBXML2_LIBS)

TargetInformationRetrieverTest_SOURCES = $(top_srcdir)/src/Test.cpp \
	TargetInformationRetrieverTest.cpp
TargetInformationRetrieverTest_CXXFLAGS = -I$(top_srcdir)/include \
	$(CPPUNIT_CFLAGS) $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS)
TargetInformationRetrieverTest_LDADD = \
	$(top_builddir)/src/hed/libs/compute/libarccompute.la \
	$(top_builddir)/src/hed/libs/common/libarccommon.la \
	$(top_builddir)/src/hed/libs/loader/libarcloader.la \
	$(CPPUNIT_LIBS) $(GLIBMM_LIBS) $(LIBXML2_LIBS)

JobListRetrieverTest_SOURCES = $(top_srcdir)/src/Test.cpp \
	JobListRetrieverTest.cpp
JobListRetrieverTest_CXXFLAGS = -I$(top_srcdir)/include \
	$(CPPUNIT_CFLAGS) $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS)
JobListRetrieverTest_LDADD = \
	$(top_builddir)/src/hed/libs/compute/libarccompute.la \
	$(top_builddir)/src/hed/libs/common/libarccommon.la \
	$(top_builddir)/src/hed/libs/loader/libarcloader.la \
	$(CPPUNIT_LIBS) $(GLIBMM_LIBS) $(LIBXML2_LIBS)

ExecutionTargetTest_SOURCES = $(top_srcdir)/src/Test.cpp \
	ExecutionTargetTest.cpp
ExecutionTargetTest_CXXFLAGS = -I$(top_srcdir)/include \
	$(CPPUNIT_CFLAGS) $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS)
ExecutionTargetTest_LDADD = \
	$(top_builddir)/src/hed/libs/compute/libarccompute.la \
	$(top_builddir)/src/hed/libs/common/libarccommon.la \
	$(top_builddir)/src/hed/libs/loader/libarcloader.la \
	$(CPPUNIT_LIBS) $(GLIBMM_LIBS) $(LIBXML2_LIBS)

ComputingServiceUniqTest_SOURCES = $(top_srcdir)/src/Test.cpp \
	ComputingServiceUniqTest.cpp
ComputingServiceUniqTest_CXXFLAGS = -I$(top_srcdir)/include \
	$(CPPUNIT_CFLAGS) $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS)
ComputingServiceUniqTest_LDADD = \
	$(top_builddir)/src/hed/libs/compute/libarccompute.la \
	$(CPPUNIT_LIBS) $(GLIBMM_LIBS) $(LIBXML2_LIBS)