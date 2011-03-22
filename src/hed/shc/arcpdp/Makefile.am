SUBDIRS = schema

noinst_LTLIBRARIES = libarcpdp.la

libarcpdp_la_SOURCES = \
	ArcPDP.cpp ArcPDP.h ArcEvaluationCtx.cpp ArcEvaluator.cpp \
	ArcRequest.cpp ArcRequestItem.cpp ArcAttributeFactory.cpp \
	ArcPolicy.cpp ArcRule.cpp ArcFnFactory.cpp ArcAlgFactory.cpp \
	ArcEvaluationCtx.h ArcEvaluator.h ArcRequest.h ArcRequestItem.h \
	ArcAttributeFactory.h ArcPolicy.h ArcRule.h ArcFnFactory.h \
	ArcAlgFactory.h ArcAttributeProxy.h
libarcpdp_la_CXXFLAGS = -I$(top_srcdir)/include \
	$(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS)
libarcpdp_la_LIBADD = \
	$(top_srcdir)/src/hed/libs/message/libarcmessage.la \
	$(top_srcdir)/src/hed/libs/common/libarccommon.la

doc_DATA = Policy.xsd Request.xsd Response.xsd
EXTRA_DIST = $(doc_DATA)
