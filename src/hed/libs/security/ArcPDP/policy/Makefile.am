#plugin
pkglib_LTLIBRARIES = libarcpolicyplugin.la
libarcpolicyplugin_la_SOURCES = PolicyParser.cpp PolicyParser.h ArcPolicy.cpp ArcPolicy.h ArcRule.cpp ArcRule.h
libarcpolicyplugin_la_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
libarcpolicyplugin_la_LIBADD=$(top_srcdir)/src/hed/libs/common/libarccommon.la $(top_srcdir)/src/hed/libs/message/libmessage.la $(top_srcdir)/src/hed/libs/loader/libarcloader.la ./libarcpolicy.la
libarcpolicyplugin_la_LDFLAGS = -no-undefined -avoid-version -module

#share library
lib_LTLIBRARIES = libarcpolicy.la
libarcpolicy_ladir = $(pkgincludedir)/security/ArcPDP/policy
libarcpolicy_la_HEADERS = Policy.h PolicyStore.h PolicyParser.h ArcPolicy.h ArcRule.h
libarcpolicy_la_SOURCES = Policy.cpp PolicyStore.cpp PolicyParser.cpp ArcPolicy.cpp ArcRule.cpp
libarcpolicy_la_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
libarcpolicy_la_LIBADD = ../attr/libarcattr.la ../fn/libarcfn.la $(top_srcdir)/src/hed/libs/common/libarccommon.la $(top_srcdir)/src/hed/libs/message/libmessage.la $(top_srcdir)/src/hed/libs/loader/libarcloader.la 
