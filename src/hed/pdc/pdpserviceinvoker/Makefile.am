
noinst_LTLIBRARIES = libarcpdpserviceinvoker.la
libarcpdpserviceinvoker_la_SOURCES  = ArcPDPServiceInvoker.cpp ArcPDPServiceInvoker.h
libarcpdpserviceinvoker_la_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
libarcpdpserviceinvoker_la_LIBADD   = $(top_srcdir)/src/hed/libs/common/libarccommon.la $(top_srcdir)/src/hed/libs/message/libarcmessage.la
libarcpdpserviceinvoker_la_LDFLAGS  = -no-undefined

