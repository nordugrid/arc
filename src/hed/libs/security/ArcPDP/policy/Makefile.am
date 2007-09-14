lib_LTLIBRARIES = libarcpolicy.la

libarcpolicy_la_SOURCES =	Policy.h \
	ArcPolicy.cpp		ArcPolicy.h \
	ArcRule.cpp		ArcRule.h \
	PolicyParser.cpp	PolicyParser.h \
	PolicyStore.cpp		PolicyStore.h
libarcpolicy_la_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) \
	-I$(top_srcdir)/include
libarcpolicy_la_LIBADD = ../attr/libarcattr.la ../fn/libarcfn.la \
	$(top_srcdir)/src/hed/libs/common/libarccommon.la
