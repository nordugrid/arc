pkglib_LTLIBRARIES = libcentralisi.la
libcentralisi_la_SOURCES = centralisi.cpp centralisi.h
libcentralisi_la_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
libcentralisi_la_LIBADD   = $(top_srcdir)/src/hed/libs/loader/libarcloader.la $(top_srcdir)/src/hed/libs/message/libarcmessage.la $(top_srcdir)/src/hed/libs/security/libarcsecurity.la $(top_srcdir)/src/hed/libs/ws/libarcws.la $(top_srcdir)/src/hed/libs/common/libarccommon.la $(GLIBMM_LIBS) $(LIBXML2_LIBS)
libcentralisi_la_LDFLAGS = -no-undefined -avoid-version -module
