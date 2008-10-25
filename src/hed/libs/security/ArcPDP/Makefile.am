SUBDIRS = attr fn policy alg
noinst_PROGRAMS = 

noinst_LTLIBRARIES = libarcpdp.la
libarcpdp_ladir = $(pkgincludedir)/security/ArcPDP
libarcpdp_la_HEADERS = Source.h   EvaluationCtx.h  Evaluator.h  Response.h Request.h RequestItem.h Result.h EvaluatorLoader.h PolicyParser.h PolicyStore.h
libarcpdp_la_SOURCES = Source.cpp EvaluationCtx.cpp Evaluator.cpp EvaluatorLoader.cpp PolicyParser.cpp PolicyStore.cpp
libarcpdp_la_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
libarcpdp_la_LIBADD = $(top_srcdir)/src/hed/libs/common/libarccommon.la  $(top_srcdir)/src/hed/libs/loader/libarcloader.la attr/libarcattr.la fn/libarcfn.la policy/libarcpolicy.la alg/libarcalg.la $(GLIBMM_LIBS)

