lib_LTLIBRARIES = libcount.la
noinst_PROGRAMS = test_client test_service
SOURCES =

libcount_la_SOURCES  = count.cpp count.h
libcount_la_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
libcount_la_LIBADD   = $(top_srcdir)/src/hed/libs/common/libarccommon.la $(top_srcdir)/src/hed/libs/loader/libarcloader.la $(top_srcdir)/src/hed/libs/message/libarcmessage.la $(LIBXML2_LIBS)
libcount_la_LDFLAGS  = -no-undefined -avoid-version -module

test_client_SOURCES  = $(SOURCES) test_client.cpp
test_client_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
test_client_LDFLAGS  = $(GLIBMM_LIBS) $(LIBXML2_LIBS)
test_client_LDADD    = $(top_srcdir)/src/hed/libs/common/libarccommon.la $(top_srcdir)/src/hed/libs/loader/libarcloader.la $(top_srcdir)/src/hed/libs/message/libarcmessage.la

test_service_SOURCES  = $(SOURCES) test_service.cpp
test_service_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
test_service_LDFLAGS  = $(GLIBMM_LIBS) $(LIBXML2_LIBS)
test_service_LDADD    = $(top_srcdir)/src/hed/libs/common/libarccommon.la $(top_srcdir)/src/hed/libs/loader/libarcloader.la $(top_srcdir)/src/hed/libs/message/libarcmessage.la

