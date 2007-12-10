noinst_LTLIBRARIES= libwssecurity.la
#noinst_PROGRAMS=test

libwssecurity_ladir = $(pkgincludedir)/ws-security
libwssecurity_la_HEADERS  = UsernameToken.h
libwssecurity_la_SOURCES  = UsernameToken.cpp
libwssecurity_la_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
libwssecurity_la_LIBADD   = $(top_srcdir)/src/hed/libs/common/libarccommon.la $(top_srcdir)/src/hed/libs/message/libarcmessage.la
libwssecurity_la_LDFLAGS  = $(LIBXML2_LIBS)


#test_SOURCES  = test.cpp 
#test_CXXFLAGS = $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
#test_LDFLAGS  = $(LIBXML2_LIBS) 
#test_LDADD    = libwssecurity.la $(top_srcdir)/src/hed/libs/common/libarccommon.la $(top_srcdir)/src/hed/libs/message/libarcmessage.la
