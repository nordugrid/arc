lib_LTLIBRARIES = libarcloader.la
noinst_LTLIBRARIES = libtestmcc.la libtestservice.la

libarcloader_ladir=$(pkgincludedir)/loader
libarcloader_la_HEADERS=Loader.h   LoaderFactory.h   MCCFactory.h   ServiceFactory.h   SecHandlerFactory.h   PDPFactory.h   DMCFactory.h   ModuleManager.h  Plexer.h    MCCLoader.h ServiceLoader.h SecHandlerLoader.h PDPLoader.h DMCLoader.h ClassLoader.h LoadableClass.h
libarcloader_la_SOURCES=Loader.cpp LoaderFactory.cpp MCCFactory.cpp ServiceFactory.cpp SecHandlerFactory.cpp PDPFactory.cpp DMCFactory.cpp ModuleManager.cpp  Plexer.cpp ClassLoader.cpp
libarcloader_la_CXXFLAGS=$(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
libarcloader_la_LIBADD=$(GLIBMM_LIBS) $(top_srcdir)/src/hed/libs/common/libarccommon.la

libtestmcc_la_SOURCES=TestMCC.cpp TestMCC.h
libtestmcc_la_CXXFLAGS=$(LIBXML2_CFLAGS) $(GLIBMM_CFLAGS) -I$(top_srcdir)/include
libtestmcc_la_LIBADD=$(GLIBMM_LIBS) $(top_srcdir)/src/hed/libs/message/libmessage.la $(top_srcdir)/src/hed/libs/common/libarccommon.la 

libtestservice_la_SOURCES=TestService.cpp TestService.h
libtestservice_la_CXXFLAGS=$(LIBXML2_CFLAGS) $(GLIBMM_CFLAGS) -I$(top_srcdir)/include

noinst_PROGRAMS=test
test_SOURCES=test.cpp
test_CXXFLAGS=$(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
test_LDFLAGS=$(GLIBMM_LIBS) $(LIBXML2_LIBS)
test_LDADD=./libarcloader.la $(top_srcdir)/src/hed/libs/common/libarccommon.la $(top_srcdir)/src/hed/libs/message/libmessage.la
