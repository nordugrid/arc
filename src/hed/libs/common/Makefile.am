DIST_SUBDIRS = test
SUBDIRS = $(TEST_DIR)

if MYSQL_LIBRARY_ENABLED
MYSQL_WRAPPER_HEADER = MysqlWrapper.h
MYSQL_WRAPPER_CPP = MysqlWrapper.cpp   
else
MYSQL_WRAPPER_HEADER =
MYSQL_WRAPPER_CPP =
endif

lib_LTLIBRARIES = libarccommon.la
libarccommon_ladir       = $(pkgincludedir)
libarccommon_la_HEADERS  = ArcConfig.h ArcLocation.h ArcRegex.h \
	Base64.h ByteArray.h DBInterface.h DateTime.h \
	FileLock.h FileUtils.h GUID.h IString.h \
	Logger.h OptionParser.h StringConv.h Thread.h URL.h \
	User.h UserConfig.h Utils.h XMLNode.h \
	Counter.h IntraProcessCounter.h IniConfig.h Profile.h \
	Run.h win32.h $(MYSQL_WRAPPER_HEADER)

if WIN32
PLATFORM_SPECIFIC=Run_win32.cpp win32.cpp
else
PLATFORM_SPECIFIC=Run_unix.cpp
endif

libarccommon_la_SOURCES  = ArcConfig.cpp ArcLocation.cpp ArcRegex.cpp \
	Base64.cpp ByteArray.cpp DBranch.cpp DBranch.h DateTime.cpp \
	FileLock.cpp FileUtils.cpp GUID.cpp IString.cpp \
	Logger.cpp OptionParser.cpp StringConv.cpp Thread.cpp URL.cpp \
	User.cpp UserConfig.cpp Utils.cpp XMLNode.cpp \
	Counter.cpp IntraProcessCounter.cpp IniConfig.cpp Profile.cpp \
	$(PLATFORM_SPECIFIC) $(MYSQL_WRAPPER_CPP)
libarccommon_la_CXXFLAGS = -I$(top_srcdir)/include \
	$(LIBXML2_CFLAGS) $(GLIBMM_CFLAGS) $(GTHREAD_CFLAGS) $(MYSQL_CFLAGS)
libarccommon_la_LIBADD   = $(LIBXML2_LIBS) $(GLIBMM_LIBS) $(GTHREAD_LIBS) \
	$(REGEX_LIBS) $(UUID_LIBS) $(EXTRA_LIBS) $(SOCKET_LIBS) \
	$(MYSQL_LIBS) $(LIBINTL)

if MYSQL_LIBRARY_ENABLED
TEST_DB_PROGRAM = test_db
else
TEST_DB_PROGRAM =
endif

noinst_PROGRAMS = testxpath test_run test_regex $(TEST_DB_PROGRAM)

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

if MYSQL_LIBRARY_ENABLED
test_db_SOURCES=test_db.cpp
test_db_CXXFLAGS=$(LIBXML2_CFLAGS) $(GLIBMM_CFLAGS) $(MYSQL_CFLAGS) -I$(top_srcdir)/include
test_db_LDFLAGS=$(LIBXML2_LIBS) $(MYSQL_LIBS) $(EXTRA_LIBS)
test_db_LDADD=./libarccommon.la
endif
