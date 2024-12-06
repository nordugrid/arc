DIST_SUBDIRS = test
SUBDIRS = $(TEST_DIR)

noinst_LTLIBRARIES = libarexrest.la

libarexrest_la_SOURCES  = rest.cpp rest.h
libarexrest_la_CXXFLAGS = -I$(top_srcdir)/include \
	$(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) $(OPENSSL_CFLAGS) $(AM_CXXFLAGS)
libarexrest_la_LIBADD = \
	$(top_builddir)/src/hed/libs/common/libarccommon.la
libarexrest_la_LDFLAGS = -no-undefined -avoid-version -module
