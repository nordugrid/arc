if MYSQL_LIBRARY_ENABLED
MYSQL_WRAPPER_TEST = MysqlWrapperTest
else
MYSQL_WRAPPER_TEST =
endif

TESTS = URLTest LoggerTest RunTest XMLNodeTest FileUtilsTest ProfileTest \
        ArcRegexTest ThreadTest FileLockTest EnvTest UserConfigTest \
        $(MYSQL_WRAPPER_TEST)

check_PROGRAMS = $(TESTS)

URLTest_SOURCES = $(top_srcdir)/src/Test.cpp URLTest.cpp
URLTest_CXXFLAGS = -I$(top_srcdir)/include \
	$(CPPUNIT_CFLAGS) $(GLIBMM_CFLAGS)
URLTest_LDADD = ../libarccommon.la $(CPPUNIT_LIBS)

LoggerTest_SOURCES = $(top_srcdir)/src/Test.cpp LoggerTest.cpp
LoggerTest_CXXFLAGS = -I$(top_srcdir)/include \
	$(CPPUNIT_CFLAGS) $(GLIBMM_CFLAGS)
LoggerTest_LDADD = ../libarccommon.la $(CPPUNIT_LIBS)

RunTest_SOURCES = $(top_srcdir)/src/Test.cpp RunTest.cpp
RunTest_CXXFLAGS = -I$(top_srcdir)/include \
	$(CPPUNIT_CFLAGS) $(GLIBMM_CFLAGS)
RunTest_LDADD = ../libarccommon.la $(CPPUNIT_LIBS)

ProfileTest_SOURCES = $(top_srcdir)/src/Test.cpp ProfileTest.cpp
ProfileTest_CXXFLAGS = -I$(top_srcdir)/include \
	$(CPPUNIT_CFLAGS) $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS)
ProfileTest_LDADD = ../libarccommon.la $(CPPUNIT_LIBS)

ThreadTest_SOURCES = $(top_srcdir)/src/Test.cpp ThreadTest.cpp
ThreadTest_CXXFLAGS = -I$(top_srcdir)/include \
	$(CPPUNIT_CFLAGS) $(GLIBMM_CFLAGS)
ThreadTest_LDADD = ../libarccommon.la $(CPPUNIT_LIBS)

XMLNodeTest_SOURCES = $(top_srcdir)/src/Test.cpp XMLNodeTest.cpp
XMLNodeTest_CXXFLAGS = -I$(top_srcdir)/include \
	$(CPPUNIT_CFLAGS) $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS)
XMLNodeTest_LDADD = ../libarccommon.la $(CPPUNIT_LIBS)

FileUtilsTest_SOURCES = $(top_srcdir)/src/Test.cpp FileUtilsTest.cpp
FileUtilsTest_CXXFLAGS = -I$(top_srcdir)/include \
	$(CPPUNIT_CFLAGS) $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS)
FileUtilsTest_LDADD = ../libarccommon.la $(CPPUNIT_LIBS)

ArcRegexTest_SOURCES = $(top_srcdir)/src/Test.cpp ArcRegexTest.cpp
ArcRegexTest_CXXFLAGS = -I$(top_srcdir)/include \
	$(CPPUNIT_CFLAGS) $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS)
ArcRegexTest_LDADD = ../libarccommon.la $(CPPUNIT_LIBS)

FileLockTest_SOURCES = $(top_srcdir)/src/Test.cpp FileLockTest.cpp
FileLockTest_CXXFLAGS = -I$(top_srcdir)/include \
	$(CPPUNIT_CFLAGS) $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS)
FileLockTest_LDADD = ../libarccommon.la $(CPPUNIT_LIBS)

EnvTest_SOURCES = $(top_srcdir)/src/Test.cpp EnvTest.cpp
EnvTest_CXXFLAGS = -I$(top_srcdir)/include \
	$(CPPUNIT_CFLAGS) $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS)
EnvTest_LDADD = ../libarccommon.la $(CPPUNIT_LIBS)

UserConfigTest_SOURCES = $(top_srcdir)/src/Test.cpp UserConfigTest.cpp
UserConfigTest_CXXFLAGS = -I$(top_srcdir)/include \
	$(CPPUNIT_CFLAGS) $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS)
UserConfigTest_LDADD = ../libarccommon.la $(CPPUNIT_LIBS)

if MYSQL_LIBRARY_ENABLED
MysqlWrapperTest_SOURCES = $(top_srcdir)/src/Test.cpp MysqlWrapperTest.cpp
MysqlWrapperTest_CXXFLAGS = -I$(top_srcdir)/include \
	$(CPPUNIT_CFLAGS) $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) $(MYSQL_CFLAGS)
MysqlWrapperTest_LDADD = ../libarccommon.la $(CPPUNIT_LIBS) $(MYSQL_LIBS)
endif

EXTRA_DIST = rcode
