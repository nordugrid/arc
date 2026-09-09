noinst_LTLIBRARIES = libfiles.la

libfiles_la_SOURCES = \
	ControlFileHandling.cpp ControlFileContent.cpp \
	ControlFileHandling.h   ControlFileContent.h
libfiles_la_CXXFLAGS = -I$(top_srcdir)/include \
	$(LIBXML2_CFLAGS) $(GLIBMM_CFLAGS) $(AM_CXXFLAGS)

TESTS = ControlFileContentTest
check_PROGRAMS = $(TESTS)
ControlFileContentTest_SOURCES = $(top_srcdir)/src/Test.cpp ControlFileContentTest.cpp
ControlFileContentTest_CXXFLAGS = -I$(top_srcdir)/include \
	$(CPPUNIT_CFLAGS) $(LIBXML2_CFLAGS) $(GLIBMM_CFLAGS) $(AM_CXXFLAGS)
ControlFileContentTest_LDADD = libfiles.la \
	$(top_builddir)/src/hed/libs/compute/libarccompute.la \
	$(top_builddir)/src/hed/libs/common/libarccommon.la \
	$(CPPUNIT_LIBS) $(GLIBMM_LIBS)
