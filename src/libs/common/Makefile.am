lib_LTLIBRARIES = libarccommon.la
libarccommon_ladir=$(pkgincludedir)
libarccommon_la_HEADERS=ArcConfig.h   XMLNode.h   Thread.h   BasicDT.h   DateTime.h   URL.h   StringConv.h   Logger.h   ArcRegex.h
libarccommon_la_SOURCES=ArcConfig.cpp XMLNode.cpp Thread.cpp BasicDT.cpp DateTime.cpp URL.cpp StringConv.cpp Logger.cpp ArcRegex.cpp
libarccommon_la_CXXFLAGS=$(LIBXML2_CFLAGS) $(GLIBMM_CFLAGS)
libarccommon_la_LDFLAGS=$(LIBXML2_LIBS) $(GLIBMM_LIBS) $(GTHREAD_LIBS)

noinst_PROGRAMS = DTtest testxpath

DTtest_SOURCES=DTtest.cpp
DTtest_CXXFLAGS=$(LIBXML2_CFLAGS) $(GLIBMM_CFLAGS)
DTtest_LDFLAGS=$(LIBXML2_LIBS)
DTtest_LDADD=./libarccommon.la

testxpath_SOURCES=testxpath.cpp
testxpath_CXXFLAGS=$(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/src/libs -I$(top_srcdir)/src/hed/libs
testxpath_LDFLAGS=$(GLIBMM_LIBS) $(LIBXML2_LIBS)
testxpath_LDADD=./libarccommon.la
