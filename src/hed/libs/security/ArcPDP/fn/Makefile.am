lib_LTLIBRARIES = libarcfn.la
libarcfn_ladir = $(pkgincludedir)/security/api/ArcPDP/fn
libarcfn_la_HEADERS = Function.h FnFactory.h
libarcfn_la_SOURCES = EqualFunction.cpp	EqualFunction.h MatchFunction.cpp MatchFunction.h InRangeFunction.cpp InRangeFunction.h
libarcfn_la_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
libarcfn_la_LIBADD = ../attr/libarcattr.la $(top_srcdir)/src/hed/libs/common/libarccommon.la
