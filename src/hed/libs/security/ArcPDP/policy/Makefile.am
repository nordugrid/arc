lib_LTLIBRARIES= libarcpolicy.la
noinst_PROGRAMS=
SOURCES =

libarcpolicy_ladir=$(includedir)
libarcpolicy_la_HEADERS=Policy.h ArcPolicy.h ArcRule.h PolicyParser.h PolicyStore.h
libarcpolicy_la_SOURCES= ArcPolicy.cpp ArcRule.cpp PolicyParser.cpp PolicyStore.cpp
libarcpolicy_la_CPPFLAGS=$(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
libarcpolicy_la_LIBADD = ../attr/libarcattr.la ../fn/libarcfn.la $(top_srcdir)/src/hed/libs/common/libarccommon.la
