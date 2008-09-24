TESTS = libarcdatatest
check_PROGRAMS = $(TESTS)

libarcdatatest_SOURCES = Test.cpp FileCacheTest.cpp
libarcdatatest_CXXFLAGS = $(CPPUNIT_CFLAGS) $(LIBXML2_CFLAGS) $(GLIBMM_CFLAGS) -I$(top_srcdir)/include
libarcdatatest_LDADD = $(CPPUNIT_LIBS) ../libarcdata2.la
