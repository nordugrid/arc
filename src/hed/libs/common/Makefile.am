DIST_SUBDIRS = test
SUBDIRS = $(TEST_DIR)

if XMLSEC_ENABLED
XMLSEC_UTILS_HEADER = XmlSecUtils.h
XMLSEC_UTILS_CPP = XmlSecUtils.cpp
else
XMLSEC_UTILS_HEADER =
XMLSEC_UTILS_CPP =
endif

if XMLSEC_ENABLED
XMLSECNODE_HEADER = XMLSecNode.h
XMLSECNODE_CPP = XMLSecNode.cpp
else
XMLSECNODE_HEADER =
XMLSECNODE_CPP =
endif

if MYSQL_LIBRARY_ENABLED
MYSQL_WRAPPER_HEADER = MysqlWrapper.h
MYSQL_WRAPPER_CPP = MysqlWrapper.cpp   
else
MYSQL_WRAPPER_HEADER =
MYSQL_WRAPPER_CPP =
endif

lib_LTLIBRARIES = libarccommon.la
libarccommon_ladir       = $(pkgincludedir)
libarccommon_la_HEADERS  = Logger.h ArcConfig.h ArcLocation.h XMLNode.h Thread.h Base64.h DateTime.h URL.h StringConv.h ArcRegex.h GUID.h Run.h User.h IString.h win32.h ByteArray.h OptionParser.h FileLock.h Utils.h DBInterface.h Counter.h IntraProcessCounter.h $(MYSQL_WRAPPER_HEADER) $(XMLSEC_UTILS_HEADER) $(XMLSECNODE_HEADER)

if WIN32
PLATFORM_SPECIFIC=Run_win32.cpp win32.cpp
else
PLATFORM_SPECIFIC=Run_unix.cpp
endif

libarccommon_la_SOURCES  = Logger.cpp ArcConfig.cpp ArcLocation.cpp XMLNode.cpp Thread.cpp Base64.cpp DateTime.cpp URL.cpp StringConv.cpp ArcRegex.cpp GUID.cpp User.cpp IString.cpp DBranch.cpp DBranch.h ByteArray.cpp OptionParser.cpp FileLock.cpp Utils.cpp Counter.cpp IntraProcessCounter.cpp $(PLATFORM_SPECIFIC) $(MYSQL_WRAPPER_CPP) $(XMLSEC_UTILS_CPP) $(XMLSECNODE_CPP)
libarccommon_la_CXXFLAGS = $(LIBXML2_CFLAGS) $(GLIBMM_CFLAGS) $(MYSQL_CFLAGS) $(XMLSEC_CFLAGS) -I$(top_srcdir)/include
libarccommon_la_LIBADD   = $(LIBXML2_LIBS) $(GLIBMM_LIBS) $(GTHREAD_LIBS) $(REGEX_LIBS) $(UUID_LIBS) $(EXTRA_LIBS) $(SOCKET_LIBS) $(MYSQL_LIBS) $(XMLSEC_LIBS) $(LIBINTL)

if MYSQL_LIBRARY_ENABLED
TEST_DB_PROGRAM = test_db
else
TEST_DB_PROGRAM =
endif

if XMLSEC_ENABLED
TEST_XMLSECNODE_PROGRAM = test_xmlsecnode
else
TEST_XMLSECNODE_PROGRAM =
endif

noinst_PROGRAMS = testxpath test_run test_regex $(TEST_DB_PROGRAM) $(TEST_XMLSECNODE_PROGRAM)

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

if XMLSEC_ENABLED
test_xmlsecnode_SOURCES=test_xmlsecnode.cpp
test_xmlsecnode_CXXFLAGS=$(LIBXML2_CFLAGS) $(GLIBMM_CFLAGS) $(XMLSEC_CFLAGS)  -I$(top_srcdir)/include
test_xmlsecnode_LDFLAGS=$(LIBXML2_LIBS) $(XMLSEC_LIBS) $(EXTRA_LIBS)
test_xmlsecnode_LDADD=./libarccommon.la
endif

