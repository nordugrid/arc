
noinst_LTLIBRARIES = libarcauthzsh.la
libarcauthzsh_la_SOURCES  = ArcAuthZ.cpp ArcAuthZ.h
libarcauthzsh_la_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
libarcauthzsh_la_LIBADD   = $(top_srcdir)/src/hed/libs/common/libarccommon.la $(top_srcdir)/src/hed/libs/message/libarcmessage.la
libarcauthzsh_la_LDFLAGS  = -no-undefined

