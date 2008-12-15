
noinst_LTLIBRARIES = libcountpdp.la
libcountpdp_la_SOURCES  = CountPDP.cpp CountPDP.h
libcountpdp_la_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
libcountpdp_la_LIBADD   = $(top_srcdir)/src/hed/libs/common/libarccommon.la $(top_srcdir)/src/hed/libs/message/libarcmessage.la
libcountpdp_la_LDFLAGS  = -no-undefined

