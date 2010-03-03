pkglib_LTLIBRARIES = libmodcrypto.la

libmodcrypto_la_SOURCES  = crypto.cpp
libmodcrypto_la_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
libmodcrypto_la_LIBADD   = $(top_srcdir)/src/hed/libs/crypto/libarccrypto.la
libmodcrypto_la_LDFLAGS  = -no-undefined -avoid-version -module
