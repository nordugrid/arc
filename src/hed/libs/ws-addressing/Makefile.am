noinst_LTLIBRARIES = libwsaddressing.la
noinst_PROGRAMS = test

libwsaddressing_ladir = $(pkgincludedir)/ws-addressing
libwsaddressing_la_HEADERS = WSA.h
libwsaddressing_la_SOURCES = WSA.cpp
libwsaddressing_la_CXXFLAGS = -I$(top_srcdir)/include \
	$(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS)
libwsaddressing_la_LIBADD = \
	$(top_srcdir)/src/hed/libs/message/libarcmessage.la \
	$(top_srcdir)/src/hed/libs/common/libarccommon.la \
	$(LIBXML2_LIBS)

test_SOURCES = test.cpp
test_CXXFLAGS = -I$(top_srcdir)/include $(LIBXML2_CFLAGS)
test_LDADD = libwsaddressing.la \
	$(top_srcdir)/src/hed/libs/message/libarcmessage.la \
	$(top_srcdir)/src/hed/libs/common/libarccommon.la \
	$(LIBXML2_LIBS)
