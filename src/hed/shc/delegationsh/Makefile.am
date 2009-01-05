
noinst_LTLIBRARIES = libdelegationsh.la
libdelegationsh_la_SOURCES  = DelegationSH.cpp DelegationSH.h
libdelegationsh_la_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) $(OPENSSL_CFLAGS) -I$(top_srcdir)/include
libdelegationsh_la_LIBADD   = $(top_srcdir)/src/hed/libs/message/libarcmessage.la $(top_srcdir)/src/hed/libs/common/libarccommon.la $(OPENSSL_LIBS)
libdelegationsh_la_LDFLAGS  = -no-undefined

