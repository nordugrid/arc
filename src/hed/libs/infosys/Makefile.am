TEST_DIR = test
DIST_SUBDIRS = test schema
SUBDIRS = schema $(TEST_DIR)

noinst_LTLIBRARIES = libinfosys.la
#noinst_PROGRAMS = test test_cache # test_register

libinfosys_ladir = $(pkgincludedir)/infosys
libinfosys_la_HEADERS = InformationInterface.h InfoCache.h \
	InfoRegister.h InfoFilter.h RegisteredService.h
libinfosys_la_SOURCES = InformationInterface.cpp InfoCache.cpp \
	InfoRegister.cpp InfoFilter.cpp RegisteredService.cpp BootstrapISIS.cpp
libinfosys_la_CXXFLAGS = -I$(top_srcdir)/include \
	$(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS)
libinfosys_la_LIBADD = \
	$(top_builddir)/src/hed/libs/client/libarcclient.la \
	$(top_builddir)/src/hed/libs/wsrf/libwsrf.la \
	$(top_builddir)/src/hed/libs/security/libarcsecurity.la \
	$(top_builddir)/src/hed/libs/message/libarcmessage.la \
	$(top_builddir)/src/hed/libs/common/libarccommon.la \
	$(LIBXML2_LIBS)

#test_SOURCES = test.cpp
#test_CXXFLAGS = -I$(top_srcdir)/include $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS)
#test_LDADD = libinfosys.la $(GLIBMM_LIBS) $(LIBXML2_LIBS)

#test_cache_SOURCES = test_cache.cpp
#test_cache_CXXFLAGS = -I$(top_srcdir)/include \
#	$(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS)
#test_cache_LDADD = libinfosys.la $(GLIBMM_LIBS) $(LIBXML2_LIBS)

#test_register_SOURCES = test_register.cpp
#test_register_CXXFLAGS = -I$(top_srcdir)/include \
#	$(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS)
#test_register_LDADD = libinfosys.la $(GLIBMM_LIBS) $(LIBXML2_LIBS) 
