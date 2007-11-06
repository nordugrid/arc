lib_LTLIBRARIES = libarcclient.la
libarcclient_ladir=$(pkgincludedir)
libarcclient_la_HEADERS=ClientInterface.h
libarcclient_la_SOURCES=ClientInterface.cpp
libarcclient_la_CXXFLAGS=$(LIBXML2_CFLAGS) $(GLIBMM_CFLAGS) -I$(top_srcdir)/include -DPKGLIBDIR='"$(pkglibdir)"'
libarcclient_la_LIBADD=$(top_srcdir)/src/hed/libs/common/libarccommon.la $(top_srcdir)/src/hed/libs/loader/libarcloader.la $(top_srcdir)/src/hed/libs/message/libarcmessage.la $(LIBXML2_LIBS) $(GLIBMM_LIBS)
