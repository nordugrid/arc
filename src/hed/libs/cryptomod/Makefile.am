pkglib_LTLIBRARIES = libmodcrypto.la

libmodcrypto_la_SOURCES = crypto.cpp
libmodcrypto_la_CXXFLAGS = -I$(top_srcdir)/include \
	$(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS)
libmodcrypto_la_LIBADD = \
	$(top_srcdir)/src/hed/libs/crypto/libarccrypto.la \
	$(top_srcdir)/src/hed/libs/common/libarccommon.la
libmodcrypto_la_LDFLAGS = -no-undefined -avoid-version -module
