
noinst_LTLIBRARIES = libdelegationsh.la
libdelegationsh_la_SOURCES  = DelegationSH.cpp DelegationSH.h
libdelegationsh_la_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
libdelegationsh_la_LIBADD   = $(top_srcdir)/src/hed/libs/message/libarcmessage.la $(top_srcdir)/src/hed/libs/common/libarccommon.la
libdelegationsh_la_LDFLAGS  = -no-undefined

