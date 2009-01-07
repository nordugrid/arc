TESTS = InformationInterfaceTest
check_PROGRAMS = $(TESTS)

InformationInterfaceTest_SOURCES = $(top_srcdir)/src/Test.cpp InformationInterfaceTest.cpp
InformationInterfaceTest_CXXFLAGS = $(CPPUNIT_CFLAGS) $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
InformationInterfaceTest_LDADD = $(CPPUNIT_LIBS) ../libinfosys.la

