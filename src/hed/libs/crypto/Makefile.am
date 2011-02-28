lib_LTLIBRARIES = libarccrypto.la

libarccrypto_ladir       = $(pkgincludedir)
libarccrypto_la_HEADERS  = OpenSSL.h
libarccrypto_la_SOURCES  = OpenSSL.cpp
libarccrypto_la_CXXFLAGS = $(GLIBMM_CFLAGS) $(OPENSSL_CFLAGS) -I$(top_srcdir)/include
libarccrypto_la_LIBADD   = $(top_srcdir)/src/hed/libs/common/libarccommon.la $(GLIBMM_LIBS) $(GTHREAD_LIBS) $(OPENSSL_LIBS)
libarccrypto_la_LDFLAGS  = -version-info 1:0:0
