noinst_LTLIBRARIES = libmisc.la

libmisc_la_SOURCES = \
	escaped.cpp canonical_dir.cpp proxy.cpp \
	escaped.h   canonical_dir.h   proxy.h
libmisc_la_CXXFLAGS = -I$(top_srcdir)/include \
	$(LIBXML2_CFLAGS) $(GLIBMM_CFLAGS)
libmisc_la_LIBADD = \
	$(top_builddir)/src/hed/libs/credentialstore/libarccredentialstore.la \
	$(top_builddir)/src/hed/libs/credential/libarccredential.la \
	$(top_builddir)/src/hed/libs/common/libarccommon.la
