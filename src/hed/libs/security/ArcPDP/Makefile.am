SUBDIRS = attr  fn policy alg
lib_LTLIBRARIES= libarcpdp.la
noinst_PROGRAMS= test
SOURCES =

libarcpdp_ladir=$(includedir)
libarcpdp_la_HEADERS=Request.h RequestItem.h Response.h Result.h ArcRequest.h ArcRequestItem.h EvaluationCtx.h Evaluator.h
libarcpdp_la_SOURCES=ArcRequest.cpp ArcRequestItem.cpp EvaluationCtx.cpp Evaluator.cpp
libarcpdp_la_CPPFLAGS=$(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
libarcpdp_la_LIBADD=attr/libarcattr.la fn/libarcfn.la policy/libarcpolicy.la alg/libarcalg.la

test_SOURCES=$(SOURCES) test.cpp
test_CPPFLAGS=$(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/src/libs  -I$(top_srcdir)/src/hed/libs
test_LDFLAGS=$(GLIBMM_LIBS) $(LIBXML2_LIBS)
test_LDADD=$(top_srcdir)/src/hed/libs/common/libarccommon.la $(top_srcdir)/src/hed/libs/loader/libarcloader.la $(top_srcdir)/src/hed/libs/message/libmessage.la $(top_srcdir)/src/hed/libs/security/ArcPDP/libarcpdp.la
