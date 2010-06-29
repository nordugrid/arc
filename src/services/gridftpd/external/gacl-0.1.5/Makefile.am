INCLUDES = $(LIBXML2_CFLAGS)

noinst_LTLIBRARIES = libgacl.la

libgacl_la_SOURCES = libgacl.c gacl.h
libgacl_la_LDFLAGS = -avoid-version

docdir   = @docdir@
doc_DATA = LICENSE.gacl

EXTRA_DIST =  $(doc_DATA)

dist-hook:
