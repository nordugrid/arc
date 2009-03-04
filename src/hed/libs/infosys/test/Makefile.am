TESTS = InformationInterfaceTest InfoFilterTest
check_PROGRAMS = $(TESTS)
noinst_PROGRAMS = ServiceISISTest

InformationInterfaceTest_SOURCES = $(top_srcdir)/src/Test.cpp InformationInterfaceTest.cpp
InformationInterfaceTest_CXXFLAGS = $(CPPUNIT_CFLAGS) $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
InformationInterfaceTest_LDADD = $(CPPUNIT_LIBS) ../libinfosys.la

InfoFilterTest_SOURCES = $(top_srcdir)/src/Test.cpp InfoFilterTest.cpp
InfoFilterTest_CXXFLAGS = $(CPPUNIT_CFLAGS) $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
InfoFilterTest_LDADD = $(CPPUNIT_LIBS) ../libinfosys.la

ServiceISISTest_SOURCES  = ServiceISISTest.cpp
ServiceISISTest_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
ServiceISISTest_LDADD    = $(top_srcdir)/src/hed/libs/message/libarcmessage.la ../libinfosys.la

