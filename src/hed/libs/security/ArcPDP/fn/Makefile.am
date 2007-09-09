lib_LTLIBRARIES= libarcfn.la
noinst_PROGRAMS=
SOURCES =

libarcfn_ladir=$(includedir)
libarcfn_la_HEADERS=Function.h EqualFunction.h MatchFunction.h FnFactory.h ArcFnFactory.h
libarcfn_la_SOURCES=EqualFunction.cpp MatchFunction.cpp ArcFnFactory.cpp
libarcfn_la_CPPFLAGS=$(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/src/libs -I$(top_srcdir)/src/hed/libs
libarcfn_la_LIBADD=../attr/libarcattr.la
