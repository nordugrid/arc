pkglib_LTLIBRARIES = libpdpservice.la
noinst_PROGRAMS=
bin_PROGRAMS=
SOURCES =

libpdpservice_la_SOURCES=pdpservice.cpp pdpservice.h
libpdpservice_la_CXXFLAGS=$(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
libpdpservice_la_LDFLAGS=$(top_srcdir)/src/hed/libs/common/libarccommon.la $(top_srcdir)/src/hed/libs/loader/libarcloader.la $(top_srcdir)/src/hed/libs/message/libmessage.la -no-undefined -avoid-version -module
