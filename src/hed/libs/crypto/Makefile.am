DIST_SUBDIRS =
SUBDIRS =
noinst_PROGRAMS =
lib_LTLIBRARIES = libarccrypto.la
pkglib_LTLIBRARIES = libmodcrypto.la

libarccrypto_ladir       = $(pkgincludedir)
libarccrypto_la_HEADERS  = OpenSSL.h
libarccrypto_la_SOURCES  = OpenSSL.cpp
libarccrypto_la_CXXFLAGS = $(GLIBMM_CFLAGS) $(OPENSSL_CFLAGS) -I$(top_srcdir)/include
libarccrypto_la_LIBADD  = $(top_srcdir)/src/hed/libs/common/libarccommon.la $(GLIBMM_LIBS) $(GTHREAD_LIBS) $(OPENSSL_LIBS)

libmodcrypto_la_SOURCES  = crypto.cpp
libmodcrypto_la_CXXFLAGS = $(GLIBMM_CFLAGS) -I$(top_srcdir)/include
libmodcrypto_la_LIBADD   = libarccrypto.la
libmodcrypto_la_LDFLAGS  = -no-undefined -avoid-version -module
