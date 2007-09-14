lib_LTLIBRARIES = libarcfn.la

libarcfn_la_SOURCES = Function.h FnFactory.h \
	EqualFunction.cpp	EqualFunction.h \
	MatchFunction.cpp	MatchFunction.h \
	InRangeFunction.cpp	InRangeFunction.h \
	ArcFnFactory.cpp	ArcFnFactory.h
libarcfn_la_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) \
	-I$(top_srcdir)/include
libarcfn_la_LIBADD = ../attr/libarcattr.la \
	$(top_srcdir)/src/hed/libs/common/libarccommon.la
