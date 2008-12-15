noinst_LTLIBRARIES=libinfosys.la
noinst_PROGRAMS=test test_cache # test_register

libinfosys_ladir = $(pkgincludedir)/infosys
libinfosys_la_HEADERS  = InformationInterface.h InfoCache.h InfoRegister.h
libinfosys_la_SOURCES  = InformationInterface.cpp InfoCache.cpp InfoRegister.cpp
libinfosys_la_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
libinfosys_la_LIBADD   = $(top_srcdir)/src/hed/libs/common/libarccommon.la $(top_srcdir)/src/hed/libs/message/libarcmessage.la $(top_srcdir)/src/hed/libs/wsrf/libwsrf.la $(top_srcdir)/src/hed/libs/client/libarcclient.la

libinfosys_la_LDFLAGS  = $(LIBXML2_LIBS)

test_SOURCES  = test.cpp
test_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
test_LDFLAGS  = $(GLIBMM_LIBS) $(LIBXML2_LIBS) 
test_LDADD    = libinfosys.la

test_cache_SOURCES  = test_cache.cpp
test_cache_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
test_cache_LDFLAGS  = $(GLIBMM_LIBS) $(LIBXML2_LIBS) 
test_cache_LDADD    = libinfosys.la
#
#test_register_SOURCES = test_register.cpp
#test_register_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
#test_register_LDFLAGS  = $(GLIBMM_LIBS) $(LIBXML2_LIBS) 
#test_register_LDADD    = libinfosys.la

