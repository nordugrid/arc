if MYSQL_LIBRARY_ENABLED
MYSQL_WRAPPER_TEST = MysqlWrapperTest
else
MYSQL_WRAPPER_TEST =
endif

TESTS = URLTest LoggerTest RunTest XMLNodeTest FileUtilsTest ProfileTest \
        ArcRegexTest ThreadTest FileLockTest EnvTest $(MYSQL_WRAPPER_TEST)

check_PROGRAMS = $(TESTS)

URLTest_SOURCES = $(top_srcdir)/src/Test.cpp URLTest.cpp
URLTest_CXXFLAGS = $(CPPUNIT_CFLAGS) $(GLIBMM_CFLAGS) -I$(top_srcdir)/include
URLTest_LDADD = $(CPPUNIT_LIBS) ../libarccommon.la

LoggerTest_SOURCES = $(top_srcdir)/src/Test.cpp LoggerTest.cpp
LoggerTest_CXXFLAGS = $(CPPUNIT_CFLAGS) $(GLIBMM_CFLAGS) -I$(top_srcdir)/include
LoggerTest_LDADD = $(CPPUNIT_LIBS) ../libarccommon.la

RunTest_SOURCES = $(top_srcdir)/src/Test.cpp RunTest.cpp
RunTest_CXXFLAGS = $(CPPUNIT_CFLAGS) $(GLIBMM_CFLAGS) -I$(top_srcdir)/include
RunTest_LDADD = $(CPPUNIT_LIBS) ../libarccommon.la

ProfileTest_SOURCES = $(top_srcdir)/src/Test.cpp ProfileTest.cpp
ProfileTest_CXXFLAGS = $(CPPUNIT_CFLAGS) $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
ProfileTest_LDADD = $(CPPUNIT_LIBS) ../libarccommon.la

ThreadTest_SOURCES = $(top_srcdir)/src/Test.cpp ThreadTest.cpp
ThreadTest_CXXFLAGS = $(CPPUNIT_CFLAGS) $(GLIBMM_CFLAGS) -I$(top_srcdir)/include
ThreadTest_LDADD = $(CPPUNIT_LIBS) ../libarccommon.la

XMLNodeTest_SOURCES = $(top_srcdir)/src/Test.cpp XMLNodeTest.cpp
XMLNodeTest_CXXFLAGS = $(CPPUNIT_CFLAGS) $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
XMLNodeTest_LDADD = $(CPPUNIT_LIBS) ../libarccommon.la

FileUtilsTest_SOURCES = $(top_srcdir)/src/Test.cpp FileUtilsTest.cpp
FileUtilsTest_CXXFLAGS = $(CPPUNIT_CFLAGS) $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
FileUtilsTest_LDADD = $(CPPUNIT_LIBS) ../libarccommon.la

ArcRegexTest_SOURCES = $(top_srcdir)/src/Test.cpp ArcRegexTest.cpp
ArcRegexTest_CXXFLAGS = $(CPPUNIT_CFLAGS) $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
ArcRegexTest_LDADD = $(CPPUNIT_LIBS) ../libarccommon.la

FileLockTest_SOURCES = $(top_srcdir)/src/Test.cpp FileLockTest.cpp
FileLockTest_CXXFLAGS = $(CPPUNIT_CFLAGS) $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
FileLockTest_LDADD = $(CPPUNIT_LIBS) ../libarccommon.la

EnvTest_SOURCES = $(top_srcdir)/src/Test.cpp EnvTest.cpp
EnvTest_CXXFLAGS = $(CPPUNIT_CFLAGS) $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
EnvTest_LDADD = $(CPPUNIT_LIBS) ../libarccommon.la

if MYSQL_LIBRARY_ENABLED
MysqlWrapperTest_SOURCES = $(top_srcdir)/src/Test.cpp MysqlWrapperTest.cpp
MysqlWrapperTest_CXXFLAGS = $(CPPUNIT_CFLAGS) $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) $(MYSQL_CFLAGS) -I$(top_srcdir)/include
MysqlWrapperTest_LDADD = $(CPPUNIT_LIBS) $(MYSQL_LIBS) ../libarccommon.la
endif

EXTRA_DIST = rcode
