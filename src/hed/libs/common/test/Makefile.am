TESTS = URLTest LoggerTest
check_PROGRAMS = $(TESTS)

URLTest_SOURCES = $(top_srcdir)/src/Test.cpp URLTest.cpp
URLTest_CXXFLAGS = $(CPPUNIT_CFLAGS) $(GLIBMM_CFLAGS) -I$(top_srcdir)/include
URLTest_LDADD = $(CPPUNIT_LIBS) ../libarccommon.la

LoggerTest_SOURCES = $(top_srcdir)/src/Test.cpp LoggerTest.cpp
LoggerTest_CXXFLAGS = $(CPPUNIT_CFLAGS) $(GLIBMM_CFLAGS) -I$(top_srcdir)/include
LoggerTest_LDADD = $(CPPUNIT_LIBS) ../libarccommon.la
