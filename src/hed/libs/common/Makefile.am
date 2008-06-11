DIST_SUBDIRS = counter test
SUBDIRS = counter $(TEST_DIR)

lib_LTLIBRARIES = libarccommon.la
libarccommon_ladir       = $(pkgincludedir)
libarccommon_la_HEADERS  = Logger.h ArcConfig.h ArcLocation.h XMLNode.h Thread.h Base64.h DateTime.h URL.h StringConv.h ArcRegex.h GUID.h Run.h User.h IString.h win32.h ByteArray.h OptionParser.h

if WIN32
PLATFORM_SPECIFIC=Run_win32.cpp win32.cpp
else
PLATFORM_SPECIFIC=Run_unix.cpp
endif

libarccommon_la_SOURCES  = Logger.cpp ArcConfig.cpp ArcLocation.cpp XMLNode.cpp Thread.cpp Base64.cpp DateTime.cpp URL.cpp StringConv.cpp ArcRegex.cpp GUID.cpp User.cpp IString.cpp DBranch.cpp DBranch.h ByteArray.cpp OptionParser.cpp $(PLATFORM_SPECIFIC)
libarccommon_la_CXXFLAGS = $(LIBXML2_CFLAGS) $(GLIBMM_CFLAGS) -I$(top_srcdir)/include
libarccommon_la_LIBADD   = counter/libcounter.la $(LIBXML2_LIBS) $(GLIBMM_LIBS) $(GTHREAD_LIBS) $(REGEX_LIBS) $(UUID_LIBS) $(EXTRA_LIBS) $(SOCKET_LIBS)

noinst_PROGRAMS = testxpath test_run test_regex

testxpath_SOURCES=testxpath.cpp
testxpath_CXXFLAGS=$(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
testxpath_LDFLAGS=$(GLIBMM_LIBS) $(LIBXML2_LIBS) 
testxpath_LDADD=./libarccommon.la

test_run_SOURCES=test_run.cpp
test_run_CXXFLAGS=$(LIBXML2_CFLAGS) $(GLIBMM_CFLAGS) -I$(top_srcdir)/include
test_run_LDFLAGS=$(LIBXML2_LIBS) $(EXTRA_LIBS)
test_run_LDADD=./libarccommon.la

test_regex_SOURCES=test_regex.cpp
test_regex_CXXFLAGS=$(LIBXML2_CFLAGS) $(GLIBMM_CFLAGS) -I$(top_srcdir)/include
test_regex_LDFLAGS=$(LIBXML2_LIBS) $(EXTRA_LIBS)
test_regex_LDADD=./libarccommon.la
