TESTS = DTRTest ProcessorTest DeliveryTest
check_PROGRAMS = $(TESTS)

DTRTest_SOURCES = $(top_srcdir)/src/Test.cpp DTRTest.cpp
DTRTest_CXXFLAGS = $(CPPUNIT_CFLAGS) $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
DTRTest_LDADD = $(CPPUNIT_LIBS) $(GLIBMM_LIBS) ../libdatastaging.la

ProcessorTest_SOURCES = $(top_srcdir)/src/Test.cpp ProcessorTest.cpp
ProcessorTest_CXXFLAGS = $(CPPUNIT_CFLAGS) $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
ProcessorTest_LDADD = $(CPPUNIT_LIBS) $(GLIBMM_LIBS) ../libdatastaging.la

DeliveryTest_SOURCES = $(top_srcdir)/src/Test.cpp DeliveryTest.cpp
DeliveryTest_CXXFLAGS = $(CPPUNIT_CFLAGS) $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
DeliveryTest_LDADD = $(CPPUNIT_LIBS) $(GLIBMM_LIBS) ../libdatastaging.la

noinst_PROGRAMS = DTRMemTest

DTRMemTest_SOURCES = DTRMemTest.cpp
DTRMemTest_CXXFLAGS = $(CPPUNIT_CFLAGS) $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
DTRMemTest_LDADD = $(CPPUNIT_LIBS) $(GLIBMM_LIBS) ../libdatastaging.la
