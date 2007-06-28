noinst_LTLIBRARIES = librsl.la

librsl_ladir=$(includedir)
librsl_la_HEADERS=parse_rsl.h   subst_rsl.h
librsl_la_SOURCES=parse_rsl.cpp subst_rsl.cpp
librsl_la_CPPFLAGS=$(LIBXML2_CFLAGS) $(GLIBMM_CFLAGS) $(GLOBUS_CFLAGS)
librsl_la_LDFLAGS=$(GLOBUS_LDFLAGS)
