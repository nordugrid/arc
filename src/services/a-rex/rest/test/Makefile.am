TESTS = RESTTest 

check_PROGRAMS = $(TESTS)

TESTS_ENVIRONMENT = srcdir=$(srcdir)

RESTTest_SOURCES = $(top_srcdir)/src/Test.cpp RESTTest.cpp
RESTTest_CXXFLAGS = -I$(top_srcdir)/include \
        -DCPPUNITTEST=1 $(CPPUNIT_CFLAGS) $(LIBXML2_CFLAGS) $(GLIBMM_CFLAGS) $(AM_CXXFLAGS)
RESTTest_LDADD = \
        $(top_builddir)/src/hed/libs/common/libarccommon.la \
        $(top_builddir)/src/hed/libs/message/libarcmessage.la \
        $(CPPUNIT_LIBS) $(GLIBMM_LIBS)
