pkglib_LTLIBRARIES = libdmchttp.la

libdmchttp_la_SOURCES = DataPointHTTP.cpp DataPointHTTP.h
libdmchttp_la_CXXFLAGS = -I$(top_srcdir)/include \
	$(LIBXML2_CFLAGS) $(GLIBMM_CFLAGS)
libdmchttp_la_LDFLAGS = -no-undefined -avoid-version -module
libdmchttp_la_LIBADD = $(top_srcdir)/src/hed/libs/data/libarcdata2.la \
	$(top_srcdir)/src/hed/libs/loader/libarcloader.la \
	$(top_srcdir)/src/hed/libs/message/libarcmessage.la \
	$(top_srcdir)/src/hed/libs/common/libarccommon.la \
	$(top_srcdir)/src/hed/libs/client/libarcclient.la \
	$(LIBXML2_LIBS) $(GLIBMM_LIBS)
