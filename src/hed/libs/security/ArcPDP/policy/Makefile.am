noinst_LTLIBRARIES = libarcpolicy.la
libarcpolicy_ladir = $(pkgincludedir)/security/ArcPDP/policy
libarcpolicy_la_HEADERS = BasePolicy.h Policy.h
libarcpolicy_la_SOURCES = Policy.cpp
libarcpolicy_la_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
libarcpolicy_la_LIBADD =  $(top_srcdir)/src/hed/libs/common/libarccommon.la
