lib_LTLIBRARIES= libarcloader.la  libtestmcc.la libtestservice.la
# mccsdir=$(topdir)
# mccs_LTLIBRARIES= 
# servicesdir=$(topdir)
# services_LTLIBRARIES= 

libarcloader_ladir=$(includedir)
libarcloader_la_HEADERS=Loader.h MCCFactory.h MCCLoader.h ModuleManager.h Plexer.h ServiceFactory.h Service.h
libarcloader_la_SOURCES=Loader.cpp MCCFactory.cpp ModuleManager.cpp Plexer.cpp ServiceFactory.cpp
libarcloader_la_CPPFLAGS=$(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/src/libs/
# libarccommon_la_LDFLAGS=

libtestmcc_la_SOURCES=TestMCC.cpp TestMCC.h
libtestmcc_la_CPPFLAGS=$(LIBXML2_CFLAGS) -I$(top_srcdir)/src/libs/
libtestmcc_la_LIBADD=../../mcc/libmcc.la

libtestservice_la_SOURCES=TestService.cpp TestService.h
libtestservice_la_CPPFLAGS=$(LIBXML2_CFLAGS) -I$(top_srcdir)/src/libs/

noinst_PROGRAMS=test
test_SOURCES=test.cpp
test_CPPFLAGS=$(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/src/libs
test_LDFLAGS=$(GLIBMM_LIBS) $(LIBXML2_LIBS)
test_LDADD=./libarcloader.la $(top_srcdir)/src/libs/common/libarccommon.la ../../mcc/libmcc.la
