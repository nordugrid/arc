
noinst_LTLIBRARIES = libgaclpdp.la
libgaclpdp_la_SOURCES  = GACLRequest.cpp GACLRequest.h GACLPolicy.cpp GACLPolicy.h GACLEvaluator.cpp GACLEvaluator.h
libgaclpdp_la_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
libgaclpdp_la_LIBADD   = $(top_srcdir)/src/hed/libs/common/libarccommon.la $(top_srcdir)/src/hed/libs/message/libarcmessage.la
libgaclpdp_la_LDFLAGS  = -no-undefined

EXTRA_DIST = $(doc_DATA)
