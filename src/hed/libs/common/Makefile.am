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
pkglibexec_PROGRAMS = arc-file-access

libarccommon_ladir = $(pkgincludedir)
libarccommon_la_HEADERS = ArcConfig.h ArcLocation.h ArcRegex.h \
	Base64.h ByteArray.h DBInterface.h DateTime.h \
	FileLock.h FileUtils.h FileAccess.h GUID.h IString.h \
	Logger.h OptionParser.h StringConv.h Thread.h URL.h \
	User.h UserConfig.h Utils.h XMLNode.h \
	Counter.h IntraProcessCounter.h IniConfig.h Profile.h \
	Run.h win32.h $(MYSQL_WRAPPER_HEADER)

if WIN32
PLATFORM_SPECIFIC = Run_win32.cpp win32.cpp
else
PLATFORM_SPECIFIC = Run_unix.cpp
endif

libarccommon_la_SOURCES = ArcConfig.cpp ArcLocation.cpp ArcRegex.cpp \
	Base64.cpp ByteArray.cpp DBranch.cpp DBranch.h DateTime.cpp \
	FileLock.cpp FileUtils.cpp FileAccess.cpp GUID.cpp IString.cpp \
	Logger.cpp OptionParser.cpp StringConv.cpp Thread.cpp URL.cpp \
	User.cpp UserConfig.cpp Utils.cpp XMLNode.cpp \
	Counter.cpp IntraProcessCounter.cpp IniConfig.cpp Profile.cpp \
	$(PLATFORM_SPECIFIC) $(MYSQL_WRAPPER_CPP)
libarccommon_la_CXXFLAGS = -I$(top_srcdir)/include \
	$(LIBXML2_CFLAGS) $(GLIBMM_CFLAGS) $(GTHREAD_CFLAGS) $(MYSQL_CFLAGS)
libarccommon_la_LIBADD = $(LIBXML2_LIBS) $(GLIBMM_LIBS) $(GTHREAD_LIBS) \
	$(REGEX_LIBS) $(UUID_LIBS) $(EXTRA_LIBS) $(SOCKET_LIBS) \
	$(MYSQL_LIBS) $(LIBINTL)
libarccommon_la_LDFLAGS = -version-info 1:0:0

arc_file_access_SOURCES  = file_access.cpp file_access.h
arc_file_access_CXXFLAGS = -I$(top_srcdir)/include \
        $(LIBXML2_CFLAGS) $(GLIBMM_CFLAGS) $(GTHREAD_CFLAGS) $(MYSQL_CFLAGS)
arc_file_access_LDADD    =
