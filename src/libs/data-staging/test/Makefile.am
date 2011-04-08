# These tests are currently disabled since they require binaries to be installed outside build tree. A new make target might be needed.
#TESTS = DTRTest ProcessorTest DeliveryTest
#check_PROGRAMS = $(TESTS)

# Since the above lines are commented, the tests below will be tried compiled even if CPPUnit is not installed. Thus put them in a if block.
if CPPUNIT_ENABLED
DTRTest_SOURCES = $(top_srcdir)/src/Test.cpp DTRTest.cpp
DTRTest_CXXFLAGS = -I$(top_srcdir)/include \
	$(CPPUNIT_CFLAGS) $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS)
DTRTest_LDADD = ../libdatastaging.la $(CPPUNIT_LIBS) $(GLIBMM_LIBS)

ProcessorTest_SOURCES = $(top_srcdir)/src/Test.cpp ProcessorTest.cpp
ProcessorTest_CXXFLAGS = -I$(top_srcdir)/include \
	$(CPPUNIT_CFLAGS) $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS)
ProcessorTest_LDADD = ../libdatastaging.la $(CPPUNIT_LIBS) $(GLIBMM_LIBS)

DeliveryTest_SOURCES = $(top_srcdir)/src/Test.cpp DeliveryTest.cpp
DeliveryTest_CXXFLAGS = -I$(top_srcdir)/include \
	$(CPPUNIT_CFLAGS) $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS)
DeliveryTest_LDADD = ../libdatastaging.la $(CPPUNIT_LIBS) $(GLIBMM_LIBS)

noinst_PROGRAMS = DTRMemTest DTRTest ProcessorTest DeliveryTest

DTRMemTest_SOURCES = DTRMemTest.cpp
DTRMemTest_CXXFLAGS = -I$(top_srcdir)/include \
	$(CPPUNIT_CFLAGS) $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS)
DTRMemTest_LDADD = ../libdatastaging.la $(CPPUNIT_LIBS) $(GLIBMM_LIBS)
endif
