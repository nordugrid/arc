SUBDIRS = attr fn policy alg
noinst_PROGRAMS = test classload_test

#plugin
pkglib_LTLIBRARIES=libarcpdpplugin.la
libarcpdpplugin_la_SOURCES= ArcRequest.cpp ArcRequestItem.cpp ArcEvaluator.cpp ArcRequest.h ArcRequestItem.h ArcEvaluator.h
libarcpdpplugin_la_CXXFLAGS=$(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
libarcpdpplugin_la_LIBADD=$(top_srcdir)/src/hed/libs/common/libarccommon.la $(top_srcdir)/src/hed/libs/message/libmessage.la $(top_srcdir)/src/hed/libs/loader/libarcloader.la ./libarcpdp.la attr/libarcattrplugin.la fn/libarcfnplugin.la policy/libarcpolicyplugin.la alg/libarcalgplugin.la
libarcpdpplugin_la_LDFLAGS = -no-undefined -avoid-version -module

#share library
lib_LTLIBRARIES = libarcpdp.la
libarcpdp_ladir = $(pkgincludedir)/security/ArcPDP
libarcpdp_la_HEADERS = EvaluationCtx.h  Evaluator.h  Response.h Request.h RequestItem.h Result.h
libarcpdp_la_SOURCES = EvaluationCtx.cpp Evaluator.cpp
libarcpdp_la_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
libarcpdp_la_LIBADD = $(top_srcdir)/src/hed/libs/common/libarccommon.la attr/libarcattr.la fn/libarcfn.la policy/libarcpolicy.la alg/libarcalg.la

test_SOURCES = test.cpp
test_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
test_LDFLAGS = $(GLIBMM_LIBS) $(LIBXML2_LIBS)
test_LDADD = $(top_srcdir)/src/hed/libs/common/libarccommon.la $(top_srcdir)/src/hed/libs/loader/libarcloader.la $(top_srcdir)/src/hed/libs/message/libmessage.la libarcpdp.la libarcpdpplugin.la attr/libarcattr.la policy/libarcpolicyplugin.la 
classload_test_SOURCES = classload_test.cpp
classload_test_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
classload_test_LDFLAGS = $(GLIBMM_LIBS) $(LIBXML2_LIBS)
classload_test_LDADD = $(top_srcdir)/src/hed/libs/common/libarccommon.la $(top_srcdir)/src/hed/libs/loader/libarcloader.la $(top_srcdir)/src/hed/libs/message/libmessage.la attr/libarcattr.la

