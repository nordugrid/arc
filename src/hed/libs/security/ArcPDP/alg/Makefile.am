lib_LTLIBRARIES= libarcalg.la
noinst_PROGRAMS=
SOURCES =

libarcalg_ladir=$(includedir)
libarcalg_la_HEADERS=AlgFactory.h ArcAlgFactory.h CombiningAlg.h DenyOverridesAlg.h PermitOverridesAlg.h
libarcalg_la_SOURCES=ArcAlgFactory.cpp DenyOverridesAlg.cpp PermitOverridesAlg.cpp
libarcalg_la_CPPFLAGS=$(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/src/libs -I$(top_srcdir)/src/hed/libs
libarcalg_la_LIBADD=../attr/libarcattr.la ../policy/libarcpolicy.la ../fn/libarcfn.la
