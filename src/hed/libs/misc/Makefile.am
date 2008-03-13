lib_LTLIBRARIES = libarcclient.la
libarcclient_ladir=$(pkgincludedir)/misc
libarcclient_la_HEADERS=ClientInterface.h   ClientTool.h
libarcclient_la_SOURCES=ClientInterface.cpp ClientTool.cpp
libarcclient_la_CXXFLAGS=$(LIBXML2_CFLAGS) $(GLIBMM_CFLAGS) -I$(top_srcdir)/include
libarcclient_la_LIBADD=$(top_srcdir)/src/hed/libs/common/libarccommon.la $(top_srcdir)/src/hed/libs/loader/libarcloader.la $(top_srcdir)/src/hed/libs/message/libarcmessage.la $(LIBXML2_LIBS) $(GLIBMM_LIBS)
