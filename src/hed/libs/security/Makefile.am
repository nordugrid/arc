SUBDIRS = ArcPDP plugin

lib_LTLIBRARIES=libsecurity.la
libsecurityapi_ladir=$(pkgincludedir)/security
libsecurityapi_la_HEADERS= SecHandler.h  PDP.h Security.h
libsecurityapi_la_SOURCES= SecHandler.cpp PDP.cpp Security.cpp
libsecurityapi_la_CXXFLAGS=$(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
libsecurityapi_la_LIBADD=$(top_srcdir)/src/hed/libs/common/libarccommon.la $(top_srcdir)/src/hed/libs/message/libmessage.la $(top_srcdir)/src/hed/libs/loade
r/libarcloader.la  ArcPDP/libarcpdp.la
libsecurityapi_la_LDFLAGS = $(LIBXML2_LIBS) $(GLIBMM_LIBS) $(GTHREAD_LIBS)

