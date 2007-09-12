noinst_LTLIBRARIES= libwsrf.la
#noinst_PROGRAMS=test

libwsrf_ladir=$(pkgincludedir)/wsrf
libwsrf_la_HEADERS=WSRF.h   WSRFBaseFault.h   WSResourceProperties.h
libwsrf_la_SOURCES=WSRF.cpp WSRFBaseFault.cpp WSResourceProperties.cpp 
libwsrf_la_CXXFLAGS=$(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
libwsrf_la_LIBADD=$(top_srcdir)/src/hed/libs/common/libarccommon.la $(top_srcdir)/src/hed/libs/message/libmessage.la $(top_srcdir)/src/hed/libs/ws-addressing/libwsaddressing.la
libwsrf_la_LDFLAGS=$(LIBXML2_LIBS)

#test_SOURCES = WSRF.cpp WSRFBaseFault.cpp WSResourceProperties.cpp
#test_CXXFLAGS=$(LIBXML2_CFLAGS) -I$(top_srcdir)/src/libs/
#test_LDFLAGS=$(LIBXML2_LIBS) 
#test_LDADD=$(top_srcdir)/src/libs/common/libarccommon.la $(top_srcdir)/src/hed/libs/message/libmessage.la
