pkglib_LTLIBRARIES = libisis.la 

AM_CPPFLAGS=@DBCXX_CPPFLAGS@

libisis_la_SOURCES = isis.cpp isis.h
libisis_la_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
libisis_la_LIBADD   = $(top_srcdir)/src/hed/libs/loader/libarcloader.la $(top_srcdir)/src/hed/libs/message/libarcmessage.la $(top_srcdir)/src/hed/libs/security/libarcsecurity.la $(top_srcdir)/src/hed/libs/ws/libarcws.la $(top_srcdir)/src/hed/libs/common/libarccommon.la $(top_srcdir)/src/hed/libs/dbxml/libarcdbxml.la $(top_srcdir)/src/hed/libs/client/libarcclient.la $(GLIBMM_LIBS)
libisis_la_LDFLAGS = -no-undefined -avoid-version -module
