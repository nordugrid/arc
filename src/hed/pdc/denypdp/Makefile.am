
noinst_LTLIBRARIES = libdenypdp.la
libdenypdp_la_SOURCES  = DenyPDP.cpp DenyPDP.h
libdenypdp_la_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
libdenypdp_la_LIBADD   = $(top_srcdir)/src/hed/libs/common/libarccommon.la $(top_srcdir)/src/hed/libs/message/libarcmessage.la
libdenypdp_la_LDFLAGS  = -no-undefined

