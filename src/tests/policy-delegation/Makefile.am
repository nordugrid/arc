noinst_PROGRAMS = test

test_SOURCES  = test.cpp
test_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) $(OPENSSL_CFLAGS) -I$(top_srcdir)/include
test_LDFLAGS  = $(GLIBMM_LIBS) $(EXTRA_LIBS)
test_LDADD    = $(top_srcdir)/src/arclib/libcredential.la
