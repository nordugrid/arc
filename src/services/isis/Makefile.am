SUBDIRS = schema

pkglib_LTLIBRARIES = libisis.la 

libisis_la_SOURCES = isis.cpp isis.h security.cpp security.h
libisis_la_CXXFLAGS = -I$(top_srcdir)/include \
	$(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) $(DBCXX_CPPFLAGS)
libisis_la_LIBADD = \
	$(top_builddir)/src/hed/libs/ws/libarcws.la \
	$(top_builddir)/src/hed/libs/dbxml/libarcdbxml.la \
	$(top_builddir)/src/hed/libs/client/libarcclient.la \
	$(top_builddir)/src/hed/libs/security/libarcsecurity.la \
	$(top_builddir)/src/hed/libs/data/libarcdata2.la \
	$(top_builddir)/src/hed/libs/message/libarcmessage.la \
	$(top_builddir)/src/hed/libs/loader/libarcloader.la \
	$(top_builddir)/src/hed/libs/common/libarccommon.la \
	$(GLIBMM_LIBS)
libisis_la_LDFLAGS = -no-undefined -avoid-version -module
