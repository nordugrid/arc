pkglib_LTLIBRARIES = libdmcarc.la

libdmcarc_la_SOURCES = DataPointARC.cpp DataPointARC.h
libdmcarc_la_CXXFLAGS = -I$(top_srcdir)/include \
	$(LIBXML2_CFLAGS) $(GLIBMM_CFLAGS)
libdmcarc_la_LDFLAGS = -no-undefined -avoid-version -module
libdmcarc_la_LIBADD = $(top_srcdir)/src/hed/libs/data/libarcdata2.la \
	$(top_srcdir)/src/hed/libs/loader/libarcloader.la \
	$(top_srcdir)/src/hed/libs/message/libarcmessage.la \
	$(top_srcdir)/src/hed/libs/common/libarccommon.la \
	$(top_srcdir)/src/hed/libs/client/libarcclient.la \
	$(LIBXML2_LIBS) $(GLIBMM_LIBS)
