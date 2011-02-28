SUBDIRS=schema
lib_LTLIBRARIES = libarcloader.la

libarcloader_ladir=$(pkgincludedir)/loader
libarcloader_la_HEADERS=Plugin.h   Loader.h   ModuleManager.h   FinderLoader.h
libarcloader_la_SOURCES=Plugin.cpp Loader.cpp ModuleManager.cpp FinderLoader.cpp
libarcloader_la_CXXFLAGS=$(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
libarcloader_la_LIBADD=$(GLIBMM_LIBS) $(LIBXML2_LIBS) $(top_srcdir)/src/hed/libs/common/libarccommon.la
libarcloader_la_LDFLAGS = -version-info 1:0:0

#libtestservice_la_SOURCES  = TestService.cpp TestService.h
#libtestservice_la_CXXFLAGS = $(LIBXML2_CFLAGS) $(GLIBMM_CFLAGS) -I$(top_srcdir)/include

#noinst_PROGRAMS = test
#test_SOURCES  = test.cpp 
#test_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
#test_LDFLAGS  = $(GLIBMM_LIBS) $(LIBXML2_LIBS) 
#test_LDADD    = ./libarcloader.la $(top_srcdir)/src/hed/libs/common/libarccommon.la $(top_srcdir)/src/hed/libs/message/libarcmessage.la 
