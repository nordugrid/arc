#pkglibexec_PROGRAMS = arc-file-access
noinst_PROGRAMS = arc-file-access ftest

#lib_LTLIBRARIES = libfileaccess.la

arc_file_access_SOURCES  = file_access.cpp file_access.h
arc_file_access_CXXFLAGS =
arc_file_access_LDADD    =

#ftest_SOURCES  = ftest.cpp
#ftest_CXXFLAGS =
#ftest_LDADD    =

ftest_SOURCES = FileAccess.cpp ftest.cpp
#ftest_HEADERS = FileAccess.h
ftest_CXXFLAGS = -I$(top_srcdir)/include \
        $(LIBXML2_CFLAGS) $(GLIBMM_CFLAGS) $(GTHREAD_CFLAGS) $(MYSQL_CFLAGS)
ftest_LDADD   = $(top_srcdir)/src/hed/libs/common/libarccommon.la \
        $(LIBXML2_LIBS) $(GLIBMM_LIBS) $(GTHREAD_LIBS) \
        $(REGEX_LIBS) $(UUID_LIBS) $(EXTRA_LIBS) $(SOCKET_LIBS) \
        $(MYSQL_LIBS) $(LIBINTL)
#libfileaccess_la_LDFLAGS  = -version-info 1:0:0

