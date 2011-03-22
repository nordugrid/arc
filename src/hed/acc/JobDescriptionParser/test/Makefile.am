TESTS = ARCJSDLParserTest JDLParserTest XRSLParserTest
check_PROGRAMS = $(TESTS)

ARCJSDLParserTest_SOURCES = $(top_srcdir)/src/Test.cpp \
	ARCJSDLParserTest.cpp ../ARCJSDLParser.cpp ../ARCJSDLParser.h
ARCJSDLParserTest_CXXFLAGS = -I$(top_srcdir)/include \
	$(CPPUNIT_CFLAGS) $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS)
ARCJSDLParserTest_LDADD = \
	$(top_srcdir)/src/hed/libs/client/libarcclient.la \
	$(top_srcdir)/src/hed/libs/common/libarccommon.la \
	$(CPPUNIT_LIBS) $(GLIBMM_LIBS) $(LIBXML2_LIBS)

JDLParserTest_SOURCES = $(top_srcdir)/src/Test.cpp \
	JDLParserTest.cpp ../JDLParser.cpp ../JDLParser.h
JDLParserTest_CXXFLAGS = -I$(top_srcdir)/include \
	$(CPPUNIT_CFLAGS) $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS)
JDLParserTest_LDADD = \
	$(top_srcdir)/src/hed/libs/client/libarcclient.la \
	$(top_srcdir)/src/hed/libs/common/libarccommon.la \
	$(CPPUNIT_LIBS) $(GLIBMM_LIBS) $(LIBXML2_LIBS)

XRSLParserTest_SOURCES = $(top_srcdir)/src/Test.cpp \
	XRSLParserTest.cpp ../XRSLParser.cpp ../XRSLParser.h \
	../RSLParser.cpp ../RSLParser.h
XRSLParserTest_CXXFLAGS = -I$(top_srcdir)/include \
	$(CPPUNIT_CFLAGS) $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS)
XRSLParserTest_LDADD = \
	$(top_srcdir)/src/hed/libs/client/libarcclient.la \
	$(top_srcdir)/src/hed/libs/common/libarccommon.la \
	$(CPPUNIT_LIBS) $(GLIBMM_LIBS) $(LIBXML2_LIBS)
