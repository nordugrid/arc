TESTS = ARCJSDLParserTest JDLParserTest XRSLParserTest
check_PROGRAMS = $(TESTS)

ARCJSDLParserTest_SOURCES = $(top_srcdir)/src/Test.cpp ARCJSDLParserTest.cpp ../ARCJSDLParser.cpp ../ARCJSDLParser.h
ARCJSDLParserTest_CXXFLAGS = $(CPPUNIT_CFLAGS) $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
ARCJSDLParserTest_LDADD = $(CPPUNIT_LIBS) $(GLIBMM_LIBS) $(LIBXML2_LIBS) $(top_srcdir)/src/hed/libs/common/libarccommon.la $(top_srcdir)/src/hed/libs/client/libarcclient.la

JDLParserTest_SOURCES = $(top_srcdir)/src/Test.cpp JDLParserTest.cpp ../JDLParser.cpp ../JDLParser.h
JDLParserTest_CXXFLAGS = $(CPPUNIT_CFLAGS) $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
JDLParserTest_LDADD = $(CPPUNIT_LIBS) $(GLIBMM_LIBS) $(LIBXML2_LIBS) $(top_srcdir)/src/hed/libs/common/libarccommon.la $(top_srcdir)/src/hed/libs/client/libarcclient.la

XRSLParserTest_SOURCES = $(top_srcdir)/src/Test.cpp XRSLParserTest.cpp ../XRSLParser.cpp ../XRSLParser.h ../RSLParser.cpp ../RSLParser.h
XRSLParserTest_CXXFLAGS = $(CPPUNIT_CFLAGS) $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
XRSLParserTest_LDADD = $(CPPUNIT_LIBS) $(GLIBMM_LIBS) $(LIBXML2_LIBS) $(top_srcdir)/src/hed/libs/common/libarccommon.la $(top_srcdir)/src/hed/libs/client/libarcclient.la
