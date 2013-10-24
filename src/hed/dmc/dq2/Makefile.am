DIST_SUBDIRS = cJSON
SUBDIRS = cJSON

pkglib_LTLIBRARIES = libdmcdq2.la

libdmcdq2_la_SOURCES = DataPointDQ2.cpp DataPointDQ2.h
libdmcdq2_la_CXXFLAGS = -I$(top_srcdir)/include \
	$(LIBXML2_CFLAGS) $(GLIBMM_CFLAGS) $(AM_CXXFLAGS) $(OPENSSL_CFLAGS)
libdmcdq2_la_LIBADD = \
  cJSON/libcjson.la \
	$(top_builddir)/src/hed/libs/communication/libarccommunication.la \
	$(top_builddir)/src/hed/libs/data/libarcdata.la \
	$(top_builddir)/src/hed/libs/message/libarcmessage.la \
	$(top_builddir)/src/hed/libs/loader/libarcloader.la \
	$(top_builddir)/src/hed/libs/common/libarccommon.la \
	$(top_builddir)/src/hed/libs/credential/libarccredential.la \
	$(LIBXML2_LIBS) $(GLIBMM_LIBS) $(OPENSSL_LIBS)
libdmcdq2_la_LDFLAGS = -no-undefined -avoid-version -module
