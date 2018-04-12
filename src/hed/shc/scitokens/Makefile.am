SUBDIRS =

pkglib_LTLIBRARIES = libarcshcscitokens.la

libarcshcscitokens_la_SOURCES = SciTokensSH.cpp SciTokensSH.h

libarcshcscitokens_la_CXXFLAGS = -I$(top_srcdir)/include \
	$(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) \
	$(XMLSEC_OPENSSL_CFLAGS) $(AM_CXXFLAGS)
libarcshcscitokens_la_LIBADD = \
	$(top_builddir)/src/hed/libs/scitokens/libarcscitokens.la \
	$(top_builddir)/src/hed/libs/message/libarcmessage.la \
	$(top_builddir)/src/hed/libs/common/libarccommon.la \
	$(XMLSEC_OPENSSL_LIBS)
