pkglib_LTLIBRARIES = libhopi.la

libhopi_la_SOURCES = hopi.cpp PayloadFile.cpp hopi.h PayloadFile.h
libhopi_la_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
libhopi_la_LIBADD = $(top_srcdir)/src/hed/libs/common/libarccommon.la $(top_srcdir)/src/hed/libs/loader/libarcloader.la $(top_srcdir)/src/hed/libs/message/libarcmessage.la $(GLIBMM_LIBS)
libhopi_la_LDFLAGS = -no-defined -avoid-version -module
