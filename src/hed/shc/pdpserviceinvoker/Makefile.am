SUBDIRS = schema

noinst_LTLIBRARIES = libarcpdpserviceinvoker.la

libarcpdpserviceinvoker_la_SOURCES = PDPServiceInvoker.cpp PDPServiceInvoker.h
libarcpdpserviceinvoker_la_CXXFLAGS = -I$(top_srcdir)/include \
	$(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS)
libarcpdpserviceinvoker_la_LIBADD = \
	$(top_builddir)/src/hed/libs/message/libarcmessage.la \
	$(top_builddir)/src/hed/libs/common/libarccommon.la
