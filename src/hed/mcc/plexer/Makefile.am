lib_LTLIBRARIES= libmccplexer.la
SOURCES = MCCPlexer.cpp 

libmccplexer_ladir=$(includedir)
libmccplexer_la_HEADERS=MCCPlexer.h
libmccplexer_la_SOURCES=MCCPlexer.cpp
libmccplexer_la_CPPFLAGS=$(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/src/libs/
libmccplexer_la_LIBADD=$(top_srcdir)/src/hed/libs/message/libmessage.la
libmccplexer_la_LDFLAGS=$(LIBXML2_LIBS)
