TESTS = DelegationInterfaceTest
check_PROGRAMS = $(TESTS)

DelegationInterfaceTest_SOURCES = $(top_srcdir)/src/Test.cpp DelegationInterfaceTest.cpp
DelegationInterfaceTest_CXXFLAGS = $(CPPUNIT_CFLAGS) $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
DelegationInterfaceTest_LDADD = $(CPPUNIT_LIBS) ../libarcdelegation.la
