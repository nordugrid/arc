lib_LTLIBRARIES=libsecurity.la
SUBDIRS=
noinst_PROGRAMS=
SOURCES = 

libsecurity_ladir=$(includedir)
libsecurity_la_HEADERS=SimpleListAuthZ.h   SimpleListPDP.h   SecHandler.h  PDP.h
libsecurity_la_SOURCES=SimpleListAuthZ.cpp SimpleListPDP.cpp
libsecurity_la_CPPFLAGS=$(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/src/libs/
libsecurity_la_LIBADD=$(top_srcdir)/src/libs/common/libarccommon.la
#libsecurity_la_LDFLAGS=$(LIBXML2_LIBS)

