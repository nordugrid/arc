TESTS = SoftwareTest ARCJSDLParserTest JDLParserTest JobStateTest XRSLParserTest BrokerTest
check_PROGRAMS = $(TESTS)

SoftwareTest_SOURCES = $(top_srcdir)/src/Test.cpp SoftwareTest.cpp
SoftwareTest_CXXFLAGS = $(CPPUNIT_CFLAGS) $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
SoftwareTest_LDADD = $(CPPUNIT_LIBS) $(GLIBMM_LIBS) $(LIBXML2_LIBS) $(top_srcdir)/src/hed/libs/common/libarccommon.la ../libarcclient.la

ARCJSDLParserTest_SOURCES = $(top_srcdir)/src/Test.cpp ARCJSDLParserTest.cpp
ARCJSDLParserTest_CXXFLAGS = $(CPPUNIT_CFLAGS) $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
ARCJSDLParserTest_LDADD = $(CPPUNIT_LIBS) $(GLIBMM_LIBS) $(LIBXML2_LIBS) $(top_srcdir)/src/hed/libs/common/libarccommon.la ../libarcclient.la

JDLParserTest_SOURCES = $(top_srcdir)/src/Test.cpp JDLParserTest.cpp
JDLParserTest_CXXFLAGS = $(CPPUNIT_CFLAGS) $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
JDLParserTest_LDADD = $(CPPUNIT_LIBS) $(GLIBMM_LIBS) $(LIBXML2_LIBS) $(top_srcdir)/src/hed/libs/common/libarccommon.la ../libarcclient.la

JobStateTest_SOURCES = $(top_srcdir)/src/Test.cpp JobStateTest.cpp
JobStateTest_CXXFLAGS = $(CPPUNIT_CFLAGS) $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
JobStateTest_LDADD = $(CPPUNIT_LIBS) $(GLIBMM_LIBS) $(LIBXML2_LIBS) $(top_srcdir)/src/hed/libs/common/libarccommon.la ../libarcclient.la

XRSLParserTest_SOURCES = $(top_srcdir)/src/Test.cpp XRSLParserTest.cpp
XRSLParserTest_CXXFLAGS = $(CPPUNIT_CFLAGS) $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
XRSLParserTest_LDADD = $(CPPUNIT_LIBS) $(GLIBMM_LIBS) $(LIBXML2_LIBS) $(top_srcdir)/src/hed/libs/common/libarccommon.la ../libarcclient.la

BrokerTest_SOURCES = $(top_srcdir)/src/Test.cpp BrokerTest.cpp
BrokerTest_CXXFLAGS = $(CPPUNIT_CFLAGS) $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
BrokerTest_LDADD = $(CPPUNIT_LIBS) $(GLIBMM_LIBS) $(LIBXML2_LIBS) $(top_srcdir)/src/hed/libs/common/libarccommon.la ../libarcclient.la
