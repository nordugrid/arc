SUBDIRS =

pkglib_LTLIBRARIES = libarcshcotokens.la

libarcshcotokens_la_SOURCES = OTokensSH.cpp OTokensSH.h

libarcshcotokens_la_CXXFLAGS = -I$(top_srcdir)/include \
	$(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) \
	$(XMLSEC_OPENSSL_CFLAGS) $(AM_CXXFLAGS)
libarcshcotokens_la_LIBADD = \
	$(top_builddir)/src/hed/libs/otokens/libarcotokens.la \
	$(top_builddir)/src/hed/libs/message/libarcmessage.la \
	$(top_builddir)/src/hed/libs/common/libarccommon.la \
	$(XMLSEC_OPENSSL_LIBS)
