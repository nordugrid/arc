TESTS = ChainTest

check_LTLIBRARIES = libtestmcc.la libtestservice.la
check_PROGRAMS = $(TESTS)

libtestmcc_la_SOURCES  = TestMCC.cpp
libtestmcc_la_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
libtestmcc_la_LIBADD   = $(top_srcdir)/src/hed/libs/common/libarccommon.la $(top_srcdir)/src/hed/libs/loader/libarcloader.la ../libarcmessage.la
libtestmcc_la_LDFLAGS  = $(LIBXML2_LIBS) -no-undefined -avoid-version -module -rpath $(CURDIR)

libtestservice_la_SOURCES  = TestService.cpp
libtestservice_la_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
libtestservice_la_LIBADD   = $(top_srcdir)/src/hed/libs/common/libarccommon.la $(top_srcdir)/src/hed/libs/loader/libarcloader.la ../libarcmessage.la
libtestservice_la_LDFLAGS  = $(LIBXML2_LIBS) -no-undefined -avoid-version -module -rpath $(CURDIR)

ChainTest_SOURCES = $(top_srcdir)/src/Test.cpp ChainTest.cpp
ChainTest_CXXFLAGS = $(CPPUNIT_CFLAGS) $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
ChainTest_LDADD = $(CPPUNIT_LIBS) $(top_srcdir)/src/hed/libs/common/libarccommon.la ../libarcmessage.la

