SUBDIRS = schema

noinst_LTLIBRARIES = libdelegationsh.la

libdelegationsh_la_SOURCES = DelegationSH.cpp DelegationSH.h
libdelegationsh_la_CXXFLAGS = -I$(top_srcdir)/include \
	$(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) $(OPENSSL_CFLAGS)
libdelegationsh_la_LIBADD = \
	$(top_srcdir)/src/hed/libs/message/libarcmessage.la \
	$(top_srcdir)/src/hed/libs/common/libarccommon.la \
	$(OPENSSL_LIBS)
