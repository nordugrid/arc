
noinst_LTLIBRARIES = libarcpdp.la
libarcpdp_la_SOURCES  = ArcPDP.cpp ArcPDP.h ArcEvaluator.cpp ArcRequest.cpp ArcRequestItem.cpp ArcAttributeFactory.cpp ArcPolicy.cpp ArcRule.cpp ArcFnFactory.cpp ArcAlgFactory.cpp ArcEvaluator.h ArcRequest.h ArcRequestItem.h ArcAttributeFactory.h ArcPolicy.h ArcRule.h ArcFnFactory.h ArcAlgFactory.h ArcAttributeProxy.h
libarcpdp_la_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
libarcpdp_la_LIBADD   = $(top_srcdir)/src/hed/libs/common/libarccommon.la $(top_srcdir)/src/hed/libs/message/libarcmessage.la
libarcpdp_la_LDFLAGS  = -no-undefined

doc_DATA = Policy.xsd Request.xsd
EXTRA_DIST = $(doc_DATA)
