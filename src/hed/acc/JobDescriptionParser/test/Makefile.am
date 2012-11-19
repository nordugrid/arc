TESTS = ADLParserTest ARCJSDLParserTest JDLParserTest XRSLParserTest
check_PROGRAMS = $(TESTS)

ADLParserTest_SOURCES = $(top_srcdir)/src/Test.cpp \
	ADLParserTest.cpp ../ADLParser.cpp ../ADLParser.h
ADLParserTest_CXXFLAGS = -I$(top_srcdir)/include \
	$(CPPUNIT_CFLAGS) $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) $(AM_CXXFLAGS)
ADLParserTest_LDADD = \
	$(top_builddir)/src/hed/libs/compute/libarccompute.la \
	$(top_builddir)/src/hed/libs/common/libarccommon.la \
	$(top_builddir)/src/hed/libs/loader/libarcloader.la \
	$(CPPUNIT_LIBS) $(GLIBMM_LIBS) $(LIBXML2_LIBS)

ARCJSDLParserTest_SOURCES = $(top_srcdir)/src/Test.cpp \
	ARCJSDLParserTest.cpp ../ARCJSDLParser.cpp ../ARCJSDLParser.h
ARCJSDLParserTest_CXXFLAGS = -I$(top_srcdir)/include \
	$(CPPUNIT_CFLAGS) $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) $(AM_CXXFLAGS)
ARCJSDLParserTest_LDADD = \
	$(top_builddir)/src/hed/libs/compute/libarccompute.la \
	$(top_builddir)/src/hed/libs/common/libarccommon.la \
	$(top_builddir)/src/hed/libs/loader/libarcloader.la \
	$(CPPUNIT_LIBS) $(GLIBMM_LIBS) $(LIBXML2_LIBS)

JDLParserTest_SOURCES = $(top_srcdir)/src/Test.cpp \
	JDLParserTest.cpp ../JDLParser.cpp ../JDLParser.h
JDLParserTest_CXXFLAGS = -I$(top_srcdir)/include \
	$(CPPUNIT_CFLAGS) $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) $(AM_CXXFLAGS)
JDLParserTest_LDADD = \
	$(top_builddir)/src/hed/libs/compute/libarccompute.la \
	$(top_builddir)/src/hed/libs/common/libarccommon.la \
	$(top_builddir)/src/hed/libs/loader/libarcloader.la \
	$(CPPUNIT_LIBS) $(GLIBMM_LIBS) $(LIBXML2_LIBS)

XRSLParserTest_SOURCES = $(top_srcdir)/src/Test.cpp \
	XRSLParserTest.cpp ../XRSLParser.cpp ../XRSLParser.h \
	../RSLParser.cpp ../RSLParser.h
XRSLParserTest_CXXFLAGS = -I$(top_srcdir)/include \
	$(CPPUNIT_CFLAGS) $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) $(AM_CXXFLAGS)
XRSLParserTest_LDADD = \
	$(top_builddir)/src/hed/libs/compute/libarccompute.la \
	$(top_builddir)/src/hed/libs/common/libarccommon.la \
	$(top_builddir)/src/hed/libs/loader/libarcloader.la \
	$(CPPUNIT_LIBS) $(GLIBMM_LIBS) $(LIBXML2_LIBS)
