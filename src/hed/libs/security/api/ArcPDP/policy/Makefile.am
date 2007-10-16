lib_LTLIBRARIES = libarcpolicy.la
libarcpolicy_ladir = $(pkgincludedir)/security/api/ArcPDP/policy
libarcpolicy_la_HEADERS = Policy.h   PolicyStore.h
libarcpolicy_la_SOURCES = Policy.cpp PolicyStore.cpp
libarcpolicy_la_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
libarcpolicy_la_LIBADD = ../attr/libarcattr.la ../fn/libarcfn.la $(top_srcdir)/src/hed/libs/common/libarccommon.la
