SUBDIRS = grid-manager
lib_LTLIBRARIES= libarex.la
#noinst_PROGRAMS=test
SOURCES =

libarex_ladir=$(includedir)
libarex_la_HEADERS=arex.h
libarex_la_SOURCES=arex.cpp
libarex_la_CPPFLAGS=$(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/src/libs/

#test_SOURCES=$(SOURCES) test.cpp
#test_CPPFLAGS=$(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/src/libs/
#test_LDFLAGS=$(GLIBMM_LIBS) $(LIBXML2_LIBS)
#test_LDADD=$(top_srcdir)/src/libs/common/libarccommon.la $(top_srcdir)/src/hed/libs/loader/libarcloader.la $(top_srcdir)/src/hed/libs/message/libmessage.la
