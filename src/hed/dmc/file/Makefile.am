pkglib_LTLIBRARIES = libdmcfile.la

libdmcfile_la_SOURCES = DataPointFile.cpp DataPointFile.h
libdmcfile_la_CXXFLAGS = -I$(top_srcdir)/include \
	$(LIBXML2_CFLAGS) $(GLIBMM_CFLAGS)
libdmcfile_la_LDFLAGS = -no-undefined -avoid-version -module
libdmcfile_la_LIBADD = $(top_srcdir)/src/hed/libs/data/libarcdata2.la \
	$(top_srcdir)/src/hed/libs/loader/libarcloader.la \
	$(top_srcdir)/src/hed/libs/message/libarcmessage.la \
	$(top_srcdir)/src/hed/libs/common/libarccommon.la \
	$(LIBXML2_LIBS) $(GLIBMM_LIBS)
