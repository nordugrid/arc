TESTS = SoftwareTest JobTest JobStateTest BrokerTest JobDescriptionTest \
	JobControllerTest JobDescriptionParserTest SubmitterTest \
	TargetRetrieverTest JobSupervisorTest

check_LTLIBRARIES = libaccTEST.la libTestACCControl.la
check_PROGRAMS = $(TESTS)

SoftwareTest_SOURCES = $(top_srcdir)/src/Test.cpp SoftwareTest.cpp
SoftwareTest_CXXFLAGS = -I$(top_srcdir)/include \
	$(CPPUNIT_CFLAGS) $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS)
SoftwareTest_LDADD = \
	$(top_srcdir)/src/hed/libs/client/libarcclient.la \
	$(top_srcdir)/src/hed/libs/common/libarccommon.la \
	$(CPPUNIT_LIBS) $(GLIBMM_LIBS) $(LIBXML2_LIBS)

JobTest_SOURCES = $(top_srcdir)/src/Test.cpp JobTest.cpp
JobTest_CXXFLAGS = -I$(top_srcdir)/include \
	$(CPPUNIT_CFLAGS) $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS)
JobTest_LDADD = \
	$(top_srcdir)/src/hed/libs/client/libarcclient.la \
	$(top_srcdir)/src/hed/libs/common/libarccommon.la \
	$(CPPUNIT_LIBS) $(GLIBMM_LIBS) $(LIBXML2_LIBS)

JobDescriptionTest_SOURCES = $(top_srcdir)/src/Test.cpp JobDescriptionTest.cpp
JobDescriptionTest_CXXFLAGS = -I$(top_srcdir)/include \
	$(CPPUNIT_CFLAGS) $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS)
JobDescriptionTest_LDADD = \
	$(top_srcdir)/src/hed/libs/client/libarcclient.la \
	$(top_srcdir)/src/hed/libs/common/libarccommon.la \
	$(CPPUNIT_LIBS) $(GLIBMM_LIBS) $(LIBXML2_LIBS)

JobStateTest_SOURCES = $(top_srcdir)/src/Test.cpp JobStateTest.cpp
JobStateTest_CXXFLAGS = -I$(top_srcdir)/include \
	$(CPPUNIT_CFLAGS) $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS)
JobStateTest_LDADD = \
	$(top_srcdir)/src/hed/libs/client/libarcclient.la \
	$(top_srcdir)/src/hed/libs/common/libarccommon.la \
	$(CPPUNIT_LIBS) $(GLIBMM_LIBS) $(LIBXML2_LIBS)

BrokerTest_SOURCES = $(top_srcdir)/src/Test.cpp BrokerTest.cpp TestACCControl.h
BrokerTest_CXXFLAGS = -I$(top_srcdir)/include \
	$(CPPUNIT_CFLAGS) $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS)
BrokerTest_LDADD = \
	libTestACCControl.la \
	$(top_srcdir)/src/hed/libs/client/libarcclient.la \
	$(top_srcdir)/src/hed/libs/common/libarccommon.la \
	$(CPPUNIT_LIBS) $(GLIBMM_LIBS) $(LIBXML2_LIBS)

JobControllerTest_SOURCES = $(top_srcdir)/src/Test.cpp JobControllerTest.cpp
JobControllerTest_CXXFLAGS = -I$(top_srcdir)/include \
	$(CPPUNIT_CFLAGS) $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS)
JobControllerTest_LDADD = \
	libTestACCControl.la \
	$(top_srcdir)/src/hed/libs/client/libarcclient.la \
	$(top_srcdir)/src/hed/libs/common/libarccommon.la \
	$(CPPUNIT_LIBS) $(GLIBMM_LIBS) $(LIBXML2_LIBS)

JobSupervisorTest_SOURCES = $(top_srcdir)/src/Test.cpp \
	JobSupervisorTest.cpp
JobSupervisorTest_CXXFLAGS = -I$(top_srcdir)/include\
	$(CPPUNIT_CFLAGS) $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS)
JobSupervisorTest_LDADD = \
	libTestACCControl.la \
	$(top_srcdir)/src/hed/libs/client/libarcclient.la \
	$(top_srcdir)/src/hed/libs/common/libarccommon.la \
	$(CPPUNIT_LIBS) $(GLIBMM_LIBS) $(LIBXML2_LIBS)

JobDescriptionParserTest_SOURCES = $(top_srcdir)/src/Test.cpp \
	JobDescriptionParserTest.cpp
JobDescriptionParserTest_CXXFLAGS = -I$(top_srcdir)/include \
	$(CPPUNIT_CFLAGS) $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS)
JobDescriptionParserTest_LDADD = \
	libTestACCControl.la \
	$(top_srcdir)/src/hed/libs/client/libarcclient.la \
	$(top_srcdir)/src/hed/libs/common/libarccommon.la \
	$(CPPUNIT_LIBS) $(GLIBMM_LIBS) $(LIBXML2_LIBS)

SubmitterTest_SOURCES = $(top_srcdir)/src/Test.cpp SubmitterTest.cpp
SubmitterTest_CXXFLAGS = -I$(top_srcdir)/include \
	$(CPPUNIT_CFLAGS) $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS)
SubmitterTest_LDADD = \
	libTestACCControl.la \
	$(top_srcdir)/src/hed/libs/client/libarcclient.la \
	$(top_srcdir)/src/hed/libs/common/libarccommon.la \
	$(CPPUNIT_LIBS) $(GLIBMM_LIBS) $(LIBXML2_LIBS)

TargetRetrieverTest_SOURCES = $(top_srcdir)/src/Test.cpp \
	TargetRetrieverTest.cpp
TargetRetrieverTest_CXXFLAGS = -I$(top_srcdir)/include \
	$(CPPUNIT_CFLAGS) $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS)
TargetRetrieverTest_LDADD = \
	libTestACCControl.la \
	$(top_srcdir)/src/hed/libs/client/libarcclient.la \
	$(top_srcdir)/src/hed/libs/common/libarccommon.la \
	$(CPPUNIT_LIBS) $(GLIBMM_LIBS) $(LIBXML2_LIBS)

libaccTEST_la_SOURCES = TestACCControl.h TestACCPluginDescriptors.cpp \
	JobControllerTestACC.cpp JobControllerTestACC.h \
	JobDescriptionParserTestACC.cpp JobDescriptionParserTestACC.h \
	SubmitterTestACC.cpp SubmitterTestACC.h \
	TargetRetrieverTestACC.cpp TargetRetrieverTestACC.h \
	BrokerTestACC.cpp BrokerTestACC.h
libaccTEST_la_CXXFLAGS = -I$(top_srcdir)/include \
	$(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS)
libaccTEST_la_LIBADD = \
	libTestACCControl.la \
	$(top_srcdir)/src/hed/libs/client/libarcclient.la \
	$(top_srcdir)/src/hed/libs/message/libarcmessage.la \
	$(top_srcdir)/src/hed/libs/loader/libarcloader.la \
	$(top_srcdir)/src/hed/libs/common/libarccommon.la \
	$(LIBXML2_LIBS)
libaccTEST_la_LDFLAGS = -no-undefined -avoid-version -module -rpath $(CURDIR)

libTestACCControl_la_SOURCES = TestACCControl.h TestACCControl.cpp
libTestACCControl_la_CXXFLAGS = -I$(top_srcdir)/include \
	$(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS)
libTestACCControl_la_LIBADD = \
	$(top_srcdir)/src/hed/libs/client/libarcclient.la \
	$(top_srcdir)/src/hed/libs/common/libarccommon.la \
	$(LIBXML2_LIBS)
libTestACCControl_la_LDFLAGS = -no-undefined -avoid-version -rpath $(CURDIR)
