bin_PROGRAMS=arcserver
SOURCES=main.cpp options.cpp options.h daemon.cpp daemon.h

arcserver_SOURCES=$(SOURCES)
arcserver_CPPFLAGS=$(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/src/libs/ -I$(top_srcdir)/src/hed/libs
arcserver_LDFLAGS=$(GLIBMM_LIBS) $(LIBXML2_LIBS)
arcserver_LDADD=$(top_srcdir)/src/libs/common/libarccommon.la $(top_srcdir)/src/hed/libs/loader/libarcloader.la $(top_srcdir)/src/hed/libs/message/libmessage.la
