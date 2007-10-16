lib_LTLIBRARIES = libarcalg.la
libarcalg_ladir = $(pkgincludedir)/security/api/ArcPDP/alg
libarcalg_la_HEADERS = AlgFactory.h CombiningAlg.h
libarcalg_la_SOURCES = DenyOverridesAlg.cpp  DenyOverridesAlg.h PermitOverridesAlg.cpp	PermitOverridesAlg.h
libarcalg_la_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
libarcalg_la_LIBADD = ../attr/libarcattr.la ../policy/libarcpolicy.la ../fn/libarcfn.la
