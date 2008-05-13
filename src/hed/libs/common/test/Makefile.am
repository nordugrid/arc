TESTS = arccommontest
check_PROGRAMS = $(TESTS)

arccommontest_SOURCES = Test.cpp URLTest.cpp LoggerTest.cpp
arccommontest_CXXFLAGS = $(CPPUNIT_CFLAGS) $(GLIBMM_CFLAGS) -I$(top_srcdir)/include
arccommontest_LDADD = $(CPPUNIT_LIBS) ../libarccommon.la
