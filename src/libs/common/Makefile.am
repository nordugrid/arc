lib_LTLIBRARIES = libarccommon.la
libarccommon_ladir=$(includedir)
libarccommon_la_HEADERS=ArcConfig.h   XMLNode.h   Thread.h   BasicDT.h   URL.h   StringConv.h   Logger.h
libarccommon_la_SOURCES=ArcConfig.cpp XMLNode.cpp Thread.cpp BasicDT.cpp URL.cpp StringConv.cpp Logger.cpp
libarccommon_la_CPPFLAGS=$(LIBXML2_CFLAGS) $(GLIBMM_CFLAGS)
libarccommon_la_LDFLAGS=$(LIBXML2_LIBS) $(GLIBMM_LIBS) $(GTHREAD_LIBS)

noinst_PROGRAMS=DTtest
DTtest_SOURCES=DTtest.cpp
DTtest_CPPFLAGS=$(LIBXML2_CFLAGS) $(GLIBMM_CFLAGS)
DTtest_LDFLAGS=$(LIBXML2_LIBS)
DTtest_LDADD=./libarccommon.la
