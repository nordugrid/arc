pkglib_LTLIBRARIES = libpdpservice.la
noinst_PROGRAMS = test

libpdpservice_la_SOURCES  = pdpservice.cpp pdpservice.h
libpdpservice_la_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
libpdpservice_la_LIBADD   = $(top_srcdir)/src/hed/libs/common/libarccommon.la $(top_srcdir)/src/hed/libs/loader/libarcloader.la $(top_srcdir)/src/hed/libs/message/libarcmessage.la
libpdpservice_la_LDFLAGS  = -no-undefined -avoid-version -module

test_SOURCES  = test.cpp
test_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
test_LDADD    = $(top_srcdir)/src/hed/libs/common/libarccommon.la $(top_srcdir)/src/hed/libs/loader/libarcloader.la \
                $(top_srcdir)/src/hed/libs/message/libarcmessage.la $(GLIBMM_LIBS) $(LIBXML2_LIBS)
