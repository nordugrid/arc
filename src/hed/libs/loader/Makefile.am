lib_LTLIBRARIES = libarcloader.la libtestclass.la

libarcloader_ladir=$(includedir)
libarcloader_la_HEADERS=ModuleManager.h
libarcloader_la_SOURCES=ModuleManager.cpp
libarcloader_la_CPPFLAGS=$(GLIBMM_CFLAGS) 
# libarccommon_la_LDFLAGS=

libtestclass_la_SOURCES=TestClass.cpp TestClass.h

noinst_PROGRAMS=test
test_SOURCES=test.cpp
test_CPPFLAGS=$(GLIBMM_CFLAGS) 
test_LDFLAGS=$(GLIBMM_LIBS) 
test_LDADD=./libarcloader.la
