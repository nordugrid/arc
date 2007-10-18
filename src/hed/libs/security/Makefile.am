SUBDIRS = ArcPDP

lib_LTLIBRARIES=libsecurity.la
libsecurity_ladir=$(pkgincludedir)/security
libsecurity_la_HEADERS= SecHandler.h  PDP.h Security.h
libsecurity_la_SOURCES= SecHandler.cpp PDP.cpp Security.cpp
libsecurity_la_CXXFLAGS=$(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
libsecurity_la_LIBADD=$(top_srcdir)/src/hed/libs/common/libarccommon.la $(top_srcdir)/src/hed/libs/message/libmessage.la $(top_srcdir)/src/hed/libs/loader/libarcloader.la  ArcPDP/libarcpdp.la
libsecurity_la_LDFLAGS = $(LIBXML2_LIBS) $(GLIBMM_LIBS) $(GTHREAD_LIBS)

