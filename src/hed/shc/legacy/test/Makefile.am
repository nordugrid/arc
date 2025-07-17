TESTS = AuthOtokensTest

check_PROGRAMS = $(TESTS)

TESTS_ENVIRONMENT = srcdir=$(srcdir)

AuthOtokensTest_SOURCES = $(top_srcdir)/src/Test.cpp AuthOtokensTest.cpp
AuthOtokensTest_CXXFLAGS = -I$(top_srcdir)/include \
	$(CPPUNIT_CFLAGS) $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) $(AM_CXXFLAGS)
AuthOtokensTest_LDADD = \
	$(top_builddir)/src/hed/libs/common/libarccommon.la \
	$(top_builddir)/src/hed/libs/message/libarcmessage.la \
	$(top_builddir)/src/hed/shc/legacy/libarcshclegacy.la \
	$(CPPUNIT_LIBS) $(GLIBMM_LIBS)

