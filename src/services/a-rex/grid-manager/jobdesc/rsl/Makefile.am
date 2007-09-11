noinst_LTLIBRARIES = librsl.la

librsl_la_SOURCES=parse_rsl.cpp subst_rsl.cpp parse_rsl.h subst_rsl.h
librsl_la_CPPFLAGS=$(LIBXML2_CFLAGS) $(GLIBMM_CFLAGS) $(GLOBUS_RSL_CFLAGS)
librsl_la_LIBADD=$(top_srcdir)/src/libs/common/libarccommon.la
# ../../jobs/libjobs.la ../../files/libfiles.la
librsl_la_LDFLAGS=$(GLOBUS_RSL_LIBS)
