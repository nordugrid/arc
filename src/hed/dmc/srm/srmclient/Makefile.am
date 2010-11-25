noinst_LTLIBRARIES = libsrmclient.la 

libsrmclient_la_SOURCES = SRMClient.cpp   SRMClient.h   \
                          SRM1Client.cpp  SRM1Client.h  \
                          SRM22Client.cpp SRM22Client.h \
                          SRMURL.cpp      SRMURL.h      \
                          SRMInfo.cpp     SRMInfo.h
libsrmclient_la_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
