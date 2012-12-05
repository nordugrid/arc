noinst_LTLIBRARIES = libdmcmock.la

libdmcmock_la_SOURCES = DataPointMock.cpp DataPointMock.h
libdmcmock_la_CXXFLAGS = -I$(top_srcdir)/include \
	$(LIBXML2_CFLAGS) $(GLIBMM_CFLAGS)
libdmcmock_la_LIBADD = \
	$(top_builddir)/src/hed/libs/data/libarcdata.la \
	$(top_builddir)/src/hed/libs/message/libarcmessage.la \
	$(top_builddir)/src/hed/libs/loader/libarcloader.la \
	$(top_builddir)/src/hed/libs/common/libarccommon.la \
	$(LIBXML2_LIBS) $(GLIBMM_LIBS)
# libtool does not create the .so file if noinst is used, so rpath is used here
# to force libtool to create it
libdmcmock_la_LDFLAGS = -no-undefined -avoid-version -module -rpath $(abs_builddir)
