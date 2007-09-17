SUBDIRS = attr fn policy alg
lib_LTLIBRARIES = libarcpdp.la
noinst_PROGRAMS = test

libarcpdp_ladir = $(pkgincludedir)/security/ArcPDP
libarcpdp_la_HEADERS = ArcRequest.h   EvaluationCtx.h   Evaluator.h \
	Response.h
libarcpdp_la_SOURCES = ArcRequest.cpp EvaluationCtx.cpp Evaluator.cpp \
	ArcRequestItem.cpp ArcRequestItem.h Request.h RequestItem.h Result.h
libarcpdp_la_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) \
	-I$(top_srcdir)/include
libarcpdp_la_LIBADD = $(top_srcdir)/src/hed/libs/common/libarccommon.la \
	attr/libarcattr.la fn/libarcfn.la policy/libarcpolicy.la \
	alg/libarcalg.la

test_SOURCES = test.cpp
test_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
test_LDFLAGS = $(GLIBMM_LIBS) $(LIBXML2_LIBS)
test_LDADD = $(top_srcdir)/src/hed/libs/common/libarccommon.la \
	$(top_srcdir)/src/hed/libs/loader/libarcloader.la \
	$(top_srcdir)/src/hed/libs/message/libmessage.la \
	libarcpdp.la
