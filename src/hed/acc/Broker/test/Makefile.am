TESTS = BenchmarkBrokerTest
check_PROGRAMS = $(TESTS)

BenchmarkBrokerTest_SOURCES = $(top_srcdir)/src/Test.cpp BenchmarkBrokerTest.cpp
BenchmarkBrokerTest_CXXFLAGS = $(CPPUNIT_CFLAGS) $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
BenchmarkBrokerTest_LDADD = $(CPPUNIT_LIBS) $(GLIBMM_LIBS) $(LIBXML2_LIBS) $(top_srcdir)/src/hed/libs/common/libarccommon.la $(top_srcdir)/src/hed/libs/client/libarcclient.la

