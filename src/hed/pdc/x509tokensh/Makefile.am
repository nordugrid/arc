
noinst_LTLIBRARIES = libx509tokensh.la
libx509tokensh_la_SOURCES  = X509TokenSH.cpp X509TokenSH.h
libx509tokensh_la_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
libx509tokensh_la_LIBADD   = $(top_srcdir)/src/hed/libs/message/libarcmessage.la $(top_srcdir)/src/hed/libs/ws-security/libarcwssecurity.la $(top_srcdir)/src/hed/libs/common/libarccommon.la
libx509tokensh_la_LDFLAGS  = -no-undefined

