lib_LTLIBRARIES = libarcloader.la libtestclass.la

libarcloader_ladir=$(includedir)
libarcloader_la_HEADERS=Loader.h MCCFactory.h MCC.h ModuleManager.h
libarcloader_la_SOURCES=Loader.cpp MCCFactory.cpp ModuleManager.cpp
libarcloader_la_CPPFLAGS=$(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/src/libs/
# libarccommon_la_LDFLAGS=

libtestclass_la_SOURCES=TestClass.cpp TestClass.h
libtestclass_la_CPPFLAGS=$(LIBXML2_CFLAGS) -I$(top_srcdir)/src/libs/

noinst_PROGRAMS=test
test_SOURCES=test.cpp
test_CPPFLAGS=$(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/src/libs
test_LDFLAGS=$(GLIBMM_LIBS) $(LIBXML2_LIBS)
test_LDADD=./libarcloader.la $(top_srcdir)/src/libs/common/libarccommon.la
