
noinst_LTLIBRARIES = libdelegationpdp.la
libdelegationpdp_la_SOURCES  = DelegationPDP.cpp DelegationPDP.h
libdelegationpdp_la_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
libdelegationpdp_la_LIBADD   = $(top_srcdir)/src/hed/libs/common/libarccommon.la $(top_srcdir)/src/hed/libs/message/libarcmessage.la
libdelegationpdp_la_LDFLAGS  = -no-undefined

