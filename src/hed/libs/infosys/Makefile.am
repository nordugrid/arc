noinst_LTLIBRARIES=libinfosys.la
noinst_PROGRAMS=test

libinfosys_ladir=$(includedir)
libinfosys_la_HEADERS=InformationInterface.h
libinfosys_la_SOURCES=InformationInterface.cpp
libinfosys_la_CPPFLAGS=$(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/src/libs/
libinfosys_la_LIBADD=$(top_srcdir)/src/libs/common/libarccommon.la $(top_srcdir)/src/hed/libs/message/libmessage.la $(top_srcdir)/src/hed/libs/wsrf/libwsrf.la
libinfosys_la_LDFLAGS=$(LIBXML2_LIBS)

test_SOURCES=test.cpp
test_CPPFLAGS=$(LIBXML2_CFLAGS) -I$(top_srcdir)/src/libs/
test_LDFLAGS=$(LIBXML2_LIBS) 
test_LDADD=
