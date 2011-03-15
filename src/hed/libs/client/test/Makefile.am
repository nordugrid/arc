TESTS = SoftwareTest JobTest JobStateTest BrokerTest JobDescriptionTest JobControllerTest SubmitterTest TargetRetrieverTest

check_LTLIBRARIES = libJobControllerTestACC.la libSubmitterTestACC.la libTargetRetrieverTestACC.la
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

BrokerTest_SOURCES = $(top_srcdir)/src/Test.cpp BrokerTest.cpp
BrokerTest_CXXFLAGS = $(CPPUNIT_CFLAGS) $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
BrokerTest_LDADD = $(CPPUNIT_LIBS) $(GLIBMM_LIBS) $(LIBXML2_LIBS) $(top_srcdir)/src/hed/libs/common/libarccommon.la ../libarcclient.la

JobControllerTest_SOURCES = $(top_srcdir)/src/Test.cpp JobControllerTest.cpp
JobControllerTest_CXXFLAGS = $(CPPUNIT_CFLAGS) $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
JobControllerTest_LDADD = $(CPPUNIT_LIBS) $(GLIBMM_LIBS) $(LIBXML2_LIBS) $(top_srcdir)/src/hed/libs/common/libarccommon.la ../libarcclient.la

SubmitterTest_SOURCES = $(top_srcdir)/src/Test.cpp SubmitterTest.cpp
SubmitterTest_CXXFLAGS = $(CPPUNIT_CFLAGS) $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
SubmitterTest_LDADD = $(CPPUNIT_LIBS) $(GLIBMM_LIBS) $(LIBXML2_LIBS) $(top_srcdir)/src/hed/libs/common/libarccommon.la ../libarcclient.la

TargetRetrieverTest_SOURCES = $(top_srcdir)/src/Test.cpp TargetRetrieverTest.cpp
TargetRetrieverTest_CXXFLAGS = $(CPPUNIT_CFLAGS) $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
TargetRetrieverTest_LDADD = $(CPPUNIT_LIBS) $(GLIBMM_LIBS) $(LIBXML2_LIBS) $(top_srcdir)/src/hed/libs/common/libarccommon.la ../libarcclient.la

libJobControllerTestACC_la_SOURCES  = JobControllerTestACC.cpp JobControllerTestACC.h
libJobControllerTestACC_la_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
libJobControllerTestACC_la_LIBADD   = $(top_srcdir)/src/hed/libs/common/libarccommon.la \
                                      $(top_srcdir)/src/hed/libs/loader/libarcloader.la \
                                      $(top_srcdir)/src/hed/libs/message/libarcmessage.la \
                                      $(top_srcdir)/src/hed/libs/client/libarcclient.la
libJobControllerTestACC_la_LDFLAGS  = $(LIBXML2_LIBS) -no-undefined -avoid-version -module -rpath $(CURDIR)

libSubmitterTestACC_la_SOURCES  = SubmitterTestACC.cpp SubmitterTestACC.h
libSubmitterTestACC_la_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
libSubmitterTestACC_la_LIBADD   = $(top_srcdir)/src/hed/libs/common/libarccommon.la \
                                  $(top_srcdir)/src/hed/libs/loader/libarcloader.la \
                                  $(top_srcdir)/src/hed/libs/message/libarcmessage.la \
                                  $(top_srcdir)/src/hed/libs/client/libarcclient.la
libSubmitterTestACC_la_LDFLAGS  = $(LIBXML2_LIBS) -no-undefined -avoid-version -module -rpath $(CURDIR)

libTargetRetrieverTestACC_la_SOURCES  = TargetRetrieverTestACC.cpp TargetRetrieverTestACC.h
libTargetRetrieverTestACC_la_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
libTargetRetrieverTestACC_la_LIBADD   = $(top_srcdir)/src/hed/libs/common/libarccommon.la \
                                        $(top_srcdir)/src/hed/libs/loader/libarcloader.la \
                                        $(top_srcdir)/src/hed/libs/message/libarcmessage.la \
                                        $(top_srcdir)/src/hed/libs/client/libarcclient.la
libTargetRetrieverTestACC_la_LDFLAGS  = $(LIBXML2_LIBS) -no-undefined -avoid-version -module -rpath $(CURDIR)
