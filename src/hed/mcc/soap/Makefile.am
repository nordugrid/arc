pkglib_LTLIBRARIES = libmccsoap.la

libmccsoap_la_SOURCES  = MCCSOAP.cpp MCCSOAP.h
libmccsoap_la_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
libmccsoap_la_LIBADD   = $(top_srcdir)/src/hed/libs/common/libarccommon.la $(top_srcdir)/src/hed/libs/message/libarcmessage.la $(top_srcdir)/src/hed/libs/ws/libarcws.la $(LIBXML2_LIBS)
libmccsoap_la_LDFLAGS  = -no-undefined -avoid-version -module
