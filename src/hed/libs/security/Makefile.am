lib_LTLIBRARIES=libsecurity.la
SUBDIRS=
noinst_PROGRAMS=
SOURCES = 

libsecurity_ladir=$(includedir)
libsecurity_la_HEADERS=SimpleListAuthZ.h   SimpleListPDP.h  ArcPDP.h  SecHandler.h  PDP.h Security.h
libsecurity_la_SOURCES=SimpleListAuthZ.cpp SimpleListPDP.cpp ArcPDP.cpp SecHandler.cpp PDP.cpp Security.cpp PDPplugin.cpp
libsecurity_la_CXXFLAGS=$(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/src/libs -I$(top_srcdir)/src/hed/libs
libsecurity_la_LIBADD=$(top_srcdir)/src/libs/common/libarccommon.la $(top_srcdir)/src/hed/libs/loader/libarcloader.la $(top_srcdir)/src/hed/libs/message/libmessage.la
#libsecurity_la_LDFLAGS=$(LIBXML2_LIBS)

