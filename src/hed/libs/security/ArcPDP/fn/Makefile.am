lib_LTLIBRARIES= libarcfn.la
noinst_PROGRAMS=
SOURCES =

libarcfn_ladir=$(includedir)
libarcfn_la_HEADERS=Function.h EqualFunction.h MatchFunction.h InRangeFunction.h FnFactory.h ArcFnFactory.h
libarcfn_la_SOURCES=EqualFunction.cpp MatchFunction.cpp InRangeFunction.cpp ArcFnFactory.cpp
libarcfn_la_CXXFLAGS=$(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
libarcfn_la_LIBADD=../attr/libarcattr.la $(top_srcdir)/src/hed/libs/common/libarccommon.la
