TESTS = libarcdatatest
check_PROGRAMS = $(TESTS)

libarcdatatest_SOURCES = $(top_srcdir)/src/Test.cpp FileCacheTest.cpp
libarcdatatest_CXXFLAGS = -I$(top_srcdir)/include \
	$(CPPUNIT_CFLAGS) $(LIBXML2_CFLAGS) $(GLIBMM_CFLAGS)
libarcdatatest_LDADD = \
	$(top_srcdir)/src/hed/libs/data/libarcdata2.la \
	$(top_srcdir)/src/hed/libs/common/libarccommon.la \
	$(CPPUNIT_LIBS)
