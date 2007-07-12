lib_LTLIBRARIES = libarcloader.la libtestmcc.la libtestservice.la

libarcloader_ladir=$(includedir)
libarcloader_la_HEADERS=Loader.h   LoaderFactory.h   MCCFactory.h   ServiceFactory.h   SecHandlerFactory.h   PDPFactory.h   DMCFactory.h   ModuleManager.h   Plexer.h    MCCLoader.h ServiceLoader.h SecHandlerLoader.h PDPLoader.h DMCLoader.h
libarcloader_la_SOURCES=Loader.cpp LoaderFactory.cpp MCCFactory.cpp ServiceFactory.cpp SecHandlerFactory.cpp PDPFactory.cpp DMCFactory.cpp ModuleManager.cpp Plexer.cpp 
libarcloader_la_CPPFLAGS=$(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/src/libs -I$(top_srcdir)/src/hed/libs
libarcloader_la_LIBADD=$(GLIBMM_LIBS) $(top_srcdir)/src/libs/common/libarccommon.la

libtestmcc_la_SOURCES=TestMCC.cpp TestMCC.h
libtestmcc_la_CPPFLAGS=$(LIBXML2_CFLAGS) $(GLIBMM_CFLAGS) -I$(top_srcdir)/src/libs -I$(top_srcdir)/src/hed/libs
libtestmcc_la_LIBADD=$(GLIBMM_LIBS) $(top_srcdir)/src/hed/libs/message/libmessage.la $(top_srcdir)/src/libs/common/libarccommon.la 

libtestservice_la_SOURCES=TestService.cpp TestService.h
libtestservice_la_CPPFLAGS=$(LIBXML2_CFLAGS) $(GLIBMM_CFLAGS) -I$(top_srcdir)/src/libs -I$(top_srcdir)/src/hed/libs

noinst_PROGRAMS=test
test_SOURCES=test.cpp
test_CPPFLAGS=$(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/src/libs -I$(top_srcdir)/src/hed/libs
test_LDFLAGS=$(GLIBMM_LIBS) $(LIBXML2_LIBS)
test_LDADD=./libarcloader.la $(top_srcdir)/src/libs/common/libarccommon.la ../message/libmessage.la
