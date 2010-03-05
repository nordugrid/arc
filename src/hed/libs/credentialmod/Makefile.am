pkglib_LTLIBRARIES = libmodcredential.la

libmodcredential_la_SOURCES  = cred.cpp
libmodcredential_la_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
libmodcredential_la_LIBADD   = $(top_srcdir)/src/hed/libs/common/libarccommon.la \
			       $(top_srcdir)/src/hed/libs/credential/libarccredential.la
libmodcredential_la_LDFLAGS  = -no-undefined -avoid-version -module
