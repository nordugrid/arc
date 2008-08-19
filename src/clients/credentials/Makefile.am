bin_PROGRAMS = arcproxy

arcproxy_SOURCES  = arcproxy.cpp
arcproxy_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) $(OPENSSL_CFLAGS) -I$(top_srcdir)/include 
arcproxy_LDFLAGS  = $(GLIBMM_LIBS) $(LIBXML2_LIBS)
arcproxy_LDADD    = $(top_srcdir)/src/hed/libs/common/libarccommon.la \
                  $(top_srcdir)/src/hed/libs/delegation/libdelegation.la

man_MANS = arcproxy.1
