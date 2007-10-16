#plugin
pkglib_LTLIBRARIES = libarcfnplugin.la
libarcfnplugin_la_SOURCES = ArcFnFactory.cpp   ArcFnFactory.h
libarcfnplugin_la_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
libarcfnplugin_la_LIBADD=$(top_srcdir)/src/hed/libs/common/libarccommon.la $(top_srcdir)/src/hed/libs/message/libmessage.la $(top_srcdir)/src/hed/libs/loader/libarcloader.la ./libarcfn.la
libarcfnplugin_la_LDFLAGS = -no-undefined -avoid-version -module

#share library
lib_LTLIBRARIES = libarcfn.la
libarcfn_ladir = $(pkgincludedir)/security/ArcPDP/fn
libarcfn_la_HEADERS = Function.h FnFactory.h
libarcfn_la_SOURCES = EqualFunction.cpp	EqualFunction.h MatchFunction.cpp MatchFunction.h InRangeFunction.cpp InRangeFunction.h
libarcfn_la_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
libarcfn_la_LIBADD = ../attr/libarcattr.la $(top_srcdir)/src/hed/libs/common/libarccommon.la
