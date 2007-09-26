pkglib_LTLIBRARIES = libmcctcp.la

libmcctcp_la_SOURCES=MCCTCP.cpp PayloadTCPSocket.cpp \
                     MCCTCP.h   PayloadTCPSocket.h
libmcctcp_la_CXXFLAGS=$(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
libmcctcp_la_LIBADD=$(top_srcdir)/src/hed/libs/message/libmessage.la $(top_srcdir)/src/hed/libs/common/libarccommon.la
libmcctcp_la_LDFLAGS=$(LIBXML2_LIBS) -lpthread -no-undefined -avoid-version -module
