#plugin
pkglib_LTLIBRARIES = libarcalgplugin.la
libarcalgplugin_la_SOURCES = ArcAlgFactory.cpp ArcAlgFactory.h 
libarcalgplugin_la_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
libarcalgplugin_la_LIBADD=$(top_srcdir)/src/hed/libs/common/libarccommon.la $(top_srcdir)/src/hed/libs/message/libmessage.la $(top_srcdir)/src/hed/libs/loader/libarcloader.la ./libarcalg.la
libarcalgplugin_la_LDFLAGS = -no-undefined -avoid-version -module

#share library
lib_LTLIBRARIES = libarcalg.la
libarcalg_ladir = $(pkgincludedir)/security/ArcPDP/alg
libarcalg_la_HEADERS = AlgFactory.h CombiningAlg.h
libarcalg_la_SOURCES = DenyOverridesAlg.cpp  DenyOverridesAlg.h PermitOverridesAlg.cpp	PermitOverridesAlg.h
libarcalg_la_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
libarcalg_la_LIBADD = ../attr/libarcattr.la ../policy/libarcpolicy.la ../fn/libarcfn.la
