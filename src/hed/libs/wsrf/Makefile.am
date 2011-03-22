noinst_LTLIBRARIES = libwsrf.la
#noinst_PROGRAMS = test

libwsrf_ladir = $(pkgincludedir)/wsrf
libwsrf_la_HEADERS = WSRF.h   WSRFBaseFault.h   WSResourceProperties.h
libwsrf_la_SOURCES = WSRF.cpp WSRFBaseFault.cpp WSResourceProperties.cpp 
libwsrf_la_CXXFLAGS = -I$(top_srcdir)/include \
	$(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS)
libwsrf_la_LIBADD = \
	$(top_srcdir)/src/hed/libs/ws-addressing/libwsaddressing.la \
	$(top_srcdir)/src/hed/libs/message/libarcmessage.la \
	$(top_srcdir)/src/hed/libs/common/libarccommon.la \
	$(LIBXML2_LIBS)

#test_SOURCES = WSRF.cpp WSRFBaseFault.cpp WSResourceProperties.cpp
#test_CXXFLAGS = -I$(top_srcdir)/include $(LIBXML2_CFLAGS)
#test_LDADD = \
#	$(top_srcdir)/src/hed/libs/message/libarcmessage.la \
#	$(top_srcdir)/src/libs/common/libarccommon.la \
#	$(LIBXML2_LIBS)
