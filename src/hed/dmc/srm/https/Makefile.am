noinst_LTLIBRARIES = libhttpsclient.la

GSOAP_LIBS = -lgsoap++

libhttpsclient_la_SOURCES = HTTPSClient.cpp HTTPSClientConnector.cpp HTTPSClientSoap.cpp HTTPSHeader.cpp HTTPSNamespaces.cpp HTTPSClient.h
libhttpsclient_la_CXXFLAGS = $(GLIBMM_CFLAGS) $(GLOBUS_FTP_CLIENT_CFLAGS) $(CC_MISC_FLAGS) -I$(top_srcdir)/include
libhttpsclient_la_LIBADD = $(top_srcdir)/src/hed/libs/common/libarccommon.la \
			    $(top_srcdir)/src/hed/libs/loader/libarcloader.la \
			    $(top_srcdir)/src/hed/libs/data/libarcdata2.la \
			    $(top_srcdir)/src/hed/libs/globusutils/libarcglobusutils.la \
			    $(GLIBMM_LIBS) $(GLOBUS_FTP_CLIENT_LIBS) $(GSOAP_LIBS)
libhttpsclient_la_LDFLAGS  = -no-undefined -avoid-version -module
