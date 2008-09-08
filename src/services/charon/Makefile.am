pkglib_LTLIBRARIES = libcharon.la
noinst_PROGRAMS = test
doc_DATA = charon_service.xml.example charon_policy.xml.example

libcharon_la_SOURCES  = charon.cpp charon.h
libcharon_la_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
libcharon_la_LIBADD   = $(top_srcdir)/src/hed/libs/common/libarccommon.la \
                     $(top_srcdir)/src/hed/libs/loader/libarcloader.la \
                     $(top_srcdir)/src/hed/libs/security/libarcsecurity.la \
                     $(top_srcdir)/src/hed/libs/message/libarcmessage.la $(GLIBMM_LIBS)
libcharon_la_LDFLAGS  = -no-undefined -avoid-version -module

test_SOURCES  = test.cpp
test_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
test_LDADD    = $(top_srcdir)/src/hed/libs/common/libarccommon.la $(top_srcdir)/src/hed/libs/loader/libarcloader.la \
                $(top_srcdir)/src/hed/libs/message/libarcmessage.la $(GLIBMM_LIBS) $(LIBXML2_LIBS)

EXTRA_DIST = $(doc_DATA)
