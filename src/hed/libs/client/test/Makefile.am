TESTS = SoftwareTest JobTest JobStateTest BrokerTest JobDescriptionTest JobControllerTest JobDescriptionParserTest SubmitterTest TargetRetrieverTest

check_LTLIBRARIES = libaccTEST.la libTestACCControl.la
check_PROGRAMS = $(TESTS)

SoftwareTest_SOURCES = $(top_srcdir)/src/Test.cpp SoftwareTest.cpp
SoftwareTest_CXXFLAGS = $(CPPUNIT_CFLAGS) $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
SoftwareTest_LDADD = $(CPPUNIT_LIBS) $(GLIBMM_LIBS) $(LIBXML2_LIBS) $(top_srcdir)/src/hed/libs/common/libarccommon.la ../libarcclient.la

JobTest_SOURCES = $(top_srcdir)/src/Test.cpp JobTest.cpp
JobTest_CXXFLAGS = $(CPPUNIT_CFLAGS) $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
JobTest_LDADD = $(CPPUNIT_LIBS) $(GLIBMM_LIBS) $(LIBXML2_LIBS) $(top_srcdir)/src/hed/libs/common/libarccommon.la ../libarcclient.la

JobDescriptionTest_SOURCES = $(top_srcdir)/src/Test.cpp JobDescriptionTest.cpp
JobDescriptionTest_CXXFLAGS = $(CPPUNIT_CFLAGS) $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
JobDescriptionTest_LDADD = $(CPPUNIT_LIBS) $(GLIBMM_LIBS) $(LIBXML2_LIBS) $(top_srcdir)/src/hed/libs/common/libarccommon.la ../libarcclient.la

JobStateTest_SOURCES = $(top_srcdir)/src/Test.cpp JobStateTest.cpp
JobStateTest_CXXFLAGS = $(CPPUNIT_CFLAGS) $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
JobStateTest_LDADD = $(CPPUNIT_LIBS) $(GLIBMM_LIBS) $(LIBXML2_LIBS) $(top_srcdir)/src/hed/libs/common/libarccommon.la ../libarcclient.la

BrokerTest_SOURCES = $(top_srcdir)/src/Test.cpp BrokerTest.cpp TestACCControl.h
BrokerTest_CXXFLAGS = $(CPPUNIT_CFLAGS) $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
BrokerTest_LDADD = $(CPPUNIT_LIBS) $(GLIBMM_LIBS) $(LIBXML2_LIBS) \
                   $(top_srcdir)/src/hed/libs/common/libarccommon.la ../libarcclient.la libTestACCControl.la

JobControllerTest_SOURCES = $(top_srcdir)/src/Test.cpp JobControllerTest.cpp
JobControllerTest_CXXFLAGS = $(CPPUNIT_CFLAGS) $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
JobControllerTest_LDADD = $(CPPUNIT_LIBS) $(GLIBMM_LIBS) $(LIBXML2_LIBS) \
                          $(top_srcdir)/src/hed/libs/common/libarccommon.la ../libarcclient.la libTestACCControl.la

JobDescriptionParserTest_SOURCES = $(top_srcdir)/src/Test.cpp JobDescriptionParserTest.cpp
JobDescriptionParserTest_CXXFLAGS = $(CPPUNIT_CFLAGS) $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
JobDescriptionParserTest_LDADD = $(CPPUNIT_LIBS) $(GLIBMM_LIBS) $(LIBXML2_LIBS) \
                                 $(top_srcdir)/src/hed/libs/common/libarccommon.la ../libarcclient.la libTestACCControl.la

SubmitterTest_SOURCES = $(top_srcdir)/src/Test.cpp SubmitterTest.cpp
SubmitterTest_CXXFLAGS = $(CPPUNIT_CFLAGS) $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
SubmitterTest_LDADD = $(CPPUNIT_LIBS) $(GLIBMM_LIBS) $(LIBXML2_LIBS) \
                      $(top_srcdir)/src/hed/libs/common/libarccommon.la ../libarcclient.la libTestACCControl.la

TargetRetrieverTest_SOURCES = $(top_srcdir)/src/Test.cpp TargetRetrieverTest.cpp
TargetRetrieverTest_CXXFLAGS = $(CPPUNIT_CFLAGS) $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
TargetRetrieverTest_LDADD = $(CPPUNIT_LIBS) $(GLIBMM_LIBS) $(LIBXML2_LIBS) \
                            $(top_srcdir)/src/hed/libs/common/libarccommon.la ../libarcclient.la libTestACCControl.la

libaccTEST_la_SOURCES  = TestACCControl.h TestACCPluginDescriptors.cpp \
                         JobControllerTestACC.cpp JobControllerTestACC.h \
                         JobDescriptionParserTestACC.cpp JobDescriptionParserTestACC.h \
                         SubmitterTestACC.cpp SubmitterTestACC.h \
                         TargetRetrieverTestACC.cpp TargetRetrieverTestACC.h \
                         BrokerTestACC.cpp BrokerTestACC.h
libaccTEST_la_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
libaccTEST_la_LIBADD   = $(top_srcdir)/src/hed/libs/common/libarccommon.la \
                         $(top_srcdir)/src/hed/libs/loader/libarcloader.la \
                         $(top_srcdir)/src/hed/libs/message/libarcmessage.la \
                         $(top_srcdir)/src/hed/libs/client/libarcclient.la \
                         libTestACCControl.la
libaccTEST_la_LDFLAGS  = $(LIBXML2_LIBS) -no-undefined -avoid-version -module -rpath $(CURDIR)

libTestACCControl_la_SOURCES  = TestACCControl.h TestACCControl.cpp
libTestACCControl_la_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
libTestACCControl_la_LIBADD   = $(top_srcdir)/src/hed/libs/common/libarccommon.la \
                                $(top_srcdir)/src/hed/libs/client/libarcclient.la
libTestACCControl_la_LDFLAGS  = $(LIBXML2_LIBS) -no-undefined -avoid-version -rpath $(CURDIR)
