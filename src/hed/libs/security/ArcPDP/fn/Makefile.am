noinst_LTLIBRARIES = libarcfn.la
libarcfn_ladir = $(pkgincludedir)/security/ArcPDP/fn
libarcfn_la_HEADERS = Function.h FnFactory.h EqualFunction.h MatchFunction.h InRangeFunction.h
libarcfn_la_SOURCES = EqualFunction.cpp MatchFunction.cpp InRangeFunction.cpp
libarcfn_la_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
libarcfn_la_LIBADD = $(top_srcdir)/src/hed/libs/common/libarccommon.la
