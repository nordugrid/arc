pkglib_LTLIBRARIES = libhttpd.la

libhttpd_la_SOURCES = httpd.cpp PayloadFile.cpp
libhttpd_la_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
libhttpd_la_LIBADD = $(top_srcdir)/src/hed/libs/common/libarccommon.la $(top_srcdir)/src/hed/libs/loader/libarcloader.la $(top_srcdir)/src/hed/libs/message/libarcmessage.la $(GLIBMM_LIBS)
libhttpd_la_LDFLAGS = -no-defined -avoid-version -module
