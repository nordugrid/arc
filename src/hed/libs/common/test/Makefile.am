TESTS = URLTest LoggerTest RunTest ThreadTest XMLNodeTest
check_PROGRAMS = $(TESTS)

URLTest_SOURCES = $(top_srcdir)/src/Test.cpp URLTest.cpp
URLTest_CXXFLAGS = $(CPPUNIT_CFLAGS) $(GLIBMM_CFLAGS) -I$(top_srcdir)/include
URLTest_LDADD = $(CPPUNIT_LIBS) ../libarccommon.la

LoggerTest_SOURCES = $(top_srcdir)/src/Test.cpp LoggerTest.cpp
LoggerTest_CXXFLAGS = $(CPPUNIT_CFLAGS) $(GLIBMM_CFLAGS) -I$(top_srcdir)/include
LoggerTest_LDADD = $(CPPUNIT_LIBS) ../libarccommon.la

RunTest_SOURCES = $(top_srcdir)/src/Test.cpp RunTest.cpp
RunTest_CXXFLAGS = $(CPPUNIT_CFLAGS) $(GLIBMM_CFLAGS) -I$(top_srcdir)/include
RunTest_LDADD = $(CPPUNIT_LIBS) ../libarccommon.la

ThreadTest_SOURCES = $(top_srcdir)/src/Test.cpp ThreadTest.cpp
ThreadTest_CXXFLAGS = $(CPPUNIT_CFLAGS) $(GLIBMM_CFLAGS) -I$(top_srcdir)/include
ThreadTest_LDADD = $(CPPUNIT_LIBS) ../libarccommon.la

XMLNodeTest_SOURCES = $(top_srcdir)/src/Test.cpp XMLNodeTest.cpp
XMLNodeTest_CXXFLAGS = $(CPPUNIT_CFLAGS) $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
XMLNodeTest_LDADD = $(CPPUNIT_LIBS) ../libarccommon.la

EXTRA_DIST = rcode
