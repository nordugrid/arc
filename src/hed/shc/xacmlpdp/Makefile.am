
noinst_LTLIBRARIES = libxacmlpdp.la
libxacmlpdp_la_SOURCES  = XACMLEvaluationCtx.cpp XACMLEvaluationCtx.h XACMLEvaluator.cpp XACMLEvaluator.h XACMLRequest.cpp XACMLRequest.h XACMLPolicy.cpp XACMLPolicy.h XACMLRule.cpp XACMLRule.h XACMLTarget.cpp XACMLTarget.h AttributeDesignator.cpp AttributeDesignator.h AttributeSelector.cpp AttributeSelector.h XACMLAttributeProxy.h XACMLAttributeFactory.cpp XACMLAttributeFactory.h XACMLFnFactory.cpp XACMLFnFactory.h XACMLAlgFactory.cpp XACMLAlgFactory.h XACMLPDP.cpp XACMLPDP.h 
libxacmlpdp_la_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
libxacmlpdp_la_LIBADD   = $(top_srcdir)/src/hed/libs/common/libarccommon.la $(top_srcdir)/src/hed/libs/message/libarcmessage.la $(top_srcdir)/src/hed/libs/security/libarcsecurity.la

libxacmlpdp_la_LDFLAGS  = -no-undefined

#doc_DATA = Policy.xsd Request.xsd
#EXTRA_DIST = $(doc_DATA)
