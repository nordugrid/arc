noinst_LTLIBRARIES = libgaclpdp.la

libgaclpdp_la_SOURCES = GACLRequest.cpp GACLRequest.h \
	GACLPolicy.cpp GACLPolicy.h \
	GACLEvaluator.cpp GACLEvaluator.h \
	GACLPDP.cpp GACLPDP.h
libgaclpdp_la_CXXFLAGS = -I$(top_srcdir)/include \
	$(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS)
libgaclpdp_la_LIBADD = $(top_srcdir)/src/hed/libs/common/libarccommon.la
