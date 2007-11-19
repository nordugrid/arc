pkglib_LTLIBRARIES = libcentralii.la
libcentralii_la_SOURCES = centralii.cpp centralii.h
libcentralii_la_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
libcentralii_la_LIBADD   = $(top_srcdir)/src/hed/libs/loader/libarcloader.la $(top_srcdir)/src/hed/libs/message/libarcmessage.la $(top_srcdir)/src/hed/libs/security/libarcsecurity.la $(top_srcdir)/src/hed/libs/ws/libarcws.la $(top_srcdir)/src/hed/libs/common/libarccommon.la
libcentralii_la_LDFLAGS = -no-undefined -avoid-version -module
