SUBDIRS = attr fn policy alg
noinst_PROGRAMS = 

lib_LTLIBRARIES = libarcpdp.la
libarcpdp_ladir = $(pkgincludedir)/security/ArcPDP
libarcpdp_la_HEADERS = EvaluationCtx.h  Evaluator.h  Response.h Request.h RequestItem.h Result.h
libarcpdp_la_SOURCES = EvaluationCtx.cpp Evaluator.cpp
libarcpdp_la_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
libarcpdp_la_LIBADD = $(top_srcdir)/src/hed/libs/common/libarccommon.la attr/libarcattr.la fn/libarcfn.la policy/libarcpolicy.la alg/libarcalg.la

