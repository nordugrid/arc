pkglib_LTLIBRARIES = libdmcsrm.la

SUBDIRS = https srmclient

libdmcsrm_la_SOURCES = DataPointSRM.cpp DataPointSRM.h
libdmcsrm_la_CXXFLAGS = -I$(top_srcdir)/include \
	$(LIBXML2_CFLAGS) $(GLIBMM_CFLAGS) $(GLOBUS_IO_CFLAGS) $(GSOAP_CFLAGS)
libdmcsrm_la_LDFLAGS = -no-undefined -avoid-version -module
libdmcsrm_la_LIBADD = srmclient/libsrmclient.la
	$(top_srcdir)/src/hed/libs/data/libarcdata2.la \
	$(top_srcdir)/src/hed/libs/loader/libarcloader.la \
	$(top_srcdir)/src/hed/libs/message/libarcmessage.la \
	$(top_srcdir)/src/hed/libs/common/libarccommon.la \
	$(LIBXML2_LIBS) $(GLIBMM_LIBS)
