lib_LTLIBRARIES = libarccommon.la
libarccommon_ladir=$(includedir)
libarccommon_la_HEADERS=ArcConfig.h   XMLNode.h   Thread.h
libarccommon_la_SOURCES=ArcConfig.cpp XMLNode.cpp Thread.cpp
libarccommon_la_CPPFLAGS=$(LIBXML2_CFLAGS) $(GLIBMM_CFLAGS)
libarccommon_la_LDFLAGS=$(LIBXML2_LIBS) $(GLIBMM_LIBS) $(GTHREAD_LIBS)

noinst_PROGRAMS=test
test_SOURCES=test.cpp
test_CPPFLAGS=$(LIBXML2_CFLAGS) $(GLIBMM_CFLAGS)
test_LDFLAGS=$(LIBXML2_LIBS)
test_LDADD=./libarccommon.la
