TESTS = SoftwareVersionTest JobDescriptionTest
check_PROGRAMS = $(TESTS)

SoftwareVersionTest_SOURCES = $(top_srcdir)/src/Test.cpp SoftwareVersionTest.cpp
SoftwareVersionTest_CXXFLAGS = $(CPPUNIT_CFLAGS) $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
SoftwareVersionTest_LDADD = $(CPPUNIT_LIBS) $(GLIBMM_LIBS) $(LIBXML2_LIBS) $(top_srcdir)/src/hed/libs/common/libarccommon.la ../libarcclient.la

JobDescriptionTest_SOURCES = $(top_srcdir)/src/Test.cpp JobDescriptionTest.cpp
JobDescriptionTest_CXXFLAGS = $(CPPUNIT_CFLAGS) $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
JobDescriptionTest_LDADD = $(CPPUNIT_LIBS) $(GLIBMM_LIBS) $(LIBXML2_LIBS) $(top_srcdir)/src/hed/libs/common/libarccommon.la ../libarcclient.la
