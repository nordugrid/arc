bin_PROGRAMS = approxy

approxy_SOURCES  = approxy.cpp
approxy_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) $(OPENSSL_CFLAGS) -I$(top_srcdir)/include 
approxy_LDFLAGS  = $(GLIBMM_LIBS) $(LIBXML2_LIBS)
approxy_LDADD    = $(top_srcdir)/src/hed/libs/common/libarccommon.la $(top_srcdir)/src/hed/libs/misc/libarcclient.la   \
                  $(top_srcdir)/src/hed/libs/security/libarcsecurity.la

man_MANS = approxy.1