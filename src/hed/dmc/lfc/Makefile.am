pkglib_LTLIBRARIES = libdmclfc.la

libdmclfc_la_SOURCES = DataPointLFC.cpp DataPointLFC.h
libdmclfc_la_CXXFLAGS = -I$(top_srcdir)/include \
	$(LIBXML2_CFLAGS) $(GLIBMM_CFLAGS) $(LFC_CFLAGS)
libdmclfc_la_LDFLAGS = -no-undefined -avoid-version -module
libdmclfc_la_LIBADD = $(top_srcdir)/src/hed/libs/data/libarcdata2.la \
	$(top_srcdir)/src/hed/libs/loader/libarcloader.la \
	$(top_srcdir)/src/hed/libs/message/libarcmessage.la \
	$(top_srcdir)/src/hed/libs/common/libarccommon.la \
	$(LIBXML2_LIBS) $(GLIBMM_LIBS) $(LFC_LIBS)
