SUBDIRS = attr policy alg fn
lib_LTLIBRARIES= libarcpdp.la
noinst_PROGRAMS=
SOURCES =

libarcpdp_ladir=$(includedir)
libarcpdp_la_HEADERS=Request.h RequestItem.h Response.h Result.h ArcRequest.h ArcRequestItem.h EvaluationCtx.h Evaluator.h
libarcpdp_la_SOURCES=ArcRequest.cpp ArcRequestItem.cpp EvaluationCtx.cpp Evaluator.cpp
libarcpdp_la_CPPFLAGS=$(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/src/libs -I$(top_srcdir)/src/hed/libs
libarcpdp_la_LIBADD=attr/libarcattr.la policy/libarcpolicy.la fn/libarcfn.la alg/libarcalg.la
