SUBDIRS=soap tcp

noinst_LTLIBRARIES= libmcc.la

libmcc_ladir=$(includedir)
libmcc_la_HEADERS=MCC.h
libmcc_la_SOURCES=MCC.cpp
libmcc_la_CPPFLAGS=$(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/src/libs/
# libmcc_la_LDFLAGS=

#noinst_PROGRAMS=test
#test_SOURCES=test.cpp
#test_CPPFLAGS=$(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/src/libs
#test_LDFLAGS=$(GLIBMM_LIBS) $(LIBXML2_LIBS)
#test_LDADD=./libarcloader.la $(top_srcdir)/src/libs/common/libarccommon.la

