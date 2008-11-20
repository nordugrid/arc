if XMLSEC_ENABLED
SLCS_CLIENT_PROGRAM = arcslcs
else
SLCS_CLIENT_PROGRAM =
endif
bin_PROGRAMS = arcproxy $(SLCS_CLIENT_PROGRAM)

arcproxy_SOURCES  = arcproxy.cpp
arcproxy_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) $(OPENSSL_CFLAGS) -I$(top_srcdir)/include 
arcproxy_LDFLAGS  = $(GLIBMM_LIBS) $(LIBXML2_LIBS)
arcproxy_LDADD    = $(top_srcdir)/src/hed/libs/common/libarccommon.la \
                  $(top_srcdir)/src/hed/libs/client/libarcclient.la \
                  $(top_srcdir)/src/hed/libs/delegation/libdelegation.la

if XMLSEC_ENABLED
arcslcs_SOURCES  = arcslcs.cpp
arcslcs_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) $(OPENSSL_CFLAGS) -I$(top_srcdir)/include
arcslcs_LDFLAGS  = $(GLIBMM_LIBS) $(LIBXML2_LIBS)
arcslcs_LDADD    = $(top_srcdir)/src/hed/libs/common/libarccommon.la \
                  $(top_srcdir)/src/hed/libs/client/libarcclient.la \
                  $(top_srcdir)/src/hed/libs/credential/libarccredential.la
endif

man_MANS = arcproxy.1 arcslcs.1
