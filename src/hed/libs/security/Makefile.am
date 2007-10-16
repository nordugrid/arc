SUBDIRS = ArcPDP

#plugin
pkglib_LTLIBRARIES=libsecurityplugin.la
libsecurityplugin_la_SOURCES=SimpleListAuthZ.cpp SimpleListPDP.cpp ArcPDP.cpp PDPplugin.cpp SimpleListAuthZ.h SimpleListPDP.h  ArcPDP.h
libsecurityplugin_la_CXXFLAGS=$(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
libsecurityplugin_la_LIBADD=$(top_srcdir)/src/hed/libs/common/libarccommon.la $(top_srcdir)/src/hed/libs/message/libmessage.la $(top_srcdir)/src/hed/libs/loader/libarcloader.la ./libsecurity.la ArcPDP/libarcpdpplugin.la
libsecurityplugin_la_LDFLAGS = -no-undefined -avoid-version -module

#share library
lib_LTLIBRARIES=libsecurity.la
libsecurity_ladir=$(pkgincludedir)/security
libsecurity_la_HEADERS= SecHandler.h  PDP.h Security.h
libsecurity_la_SOURCES= SecHandler.cpp PDP.cpp Security.cpp
libsecurity_la_CXXFLAGS=$(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
libsecurity_la_LIBADD=$(top_srcdir)/src/hed/libs/common/libarccommon.la $(top_srcdir)/src/hed/libs/message/libmessage.la $(top_srcdir)/src/hed/libs/loader/libarcloader.la  ArcPDP/libarcpdp.la
libsecurity_la_LDFLAGS = $(LIBXML2_LIBS) $(GLIBMM_LIBS) $(GTHREAD_LIBS)
