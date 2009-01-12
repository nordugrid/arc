if XMLSEC_ENABLED
SLCS_CLIENT_PROGRAM = arcslcs
else
SLCS_CLIENT_PROGRAM =
endif
bin_PROGRAMS = arcproxy $(SLCS_CLIENT_PROGRAM)

noinst_PROGRAMS = test2vomsserver

arcproxy_SOURCES  = arcproxy.cpp
arcproxy_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) $(OPENSSL_CFLAGS) -I$(top_srcdir)/include 
arcproxy_LDFLAGS  = $(GLIBMM_LIBS) $(LIBXML2_LIBS)
arcproxy_LDADD    = $(top_srcdir)/src/hed/libs/common/libarccommon.la \
                  $(top_srcdir)/src/hed/libs/client/libarcclient.la \
                  $(top_srcdir)/src/hed/libs/delegation/libarcdelegation.la

if XMLSEC_ENABLED
arcslcs_SOURCES  = arcslcs.cpp
arcslcs_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) $(OPENSSL_CFLAGS) -I$(top_srcdir)/include
arcslcs_LDFLAGS  = $(GLIBMM_LIBS) $(LIBXML2_LIBS)
arcslcs_LDADD    = $(top_srcdir)/src/hed/libs/common/libarccommon.la \
                  $(top_srcdir)/src/hed/libs/client/libarcclient.la \
                  $(top_srcdir)/src/hed/libs/credential/libarccredential.la
endif

test2vomsserver_SOURCES  = test2vomsserver.cpp
test2vomsserver_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
test2vomsserver_LDFLAGS  = $(GLIBMM_LIBS) $(LIBXML2_LIBS)
test2vomsserver_LDADD    = $(top_srcdir)/src/hed/libs/common/libarccommon.la \
                           $(top_srcdir)/src/hed/libs/loader/libarcloader.la \
                           $(top_srcdir)/src/hed/libs/client/libarcclient.la \
                           $(top_srcdir)/src/hed/libs/message/libarcmessage.la

man_MANS = arcproxy.1 arcslcs.1
