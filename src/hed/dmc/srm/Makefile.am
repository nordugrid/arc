pkglib_LTLIBRARIES = libdmcsrm.la

SUBDIRS = https srmclient

libdmcsrm_la_SOURCES  = DMCSRM.cpp DataPointSRM.cpp \
                        DMCSRM.h   DataPointSRM.h
libdmcsrm_la_CXXFLAGS = $(LIBXML2_CFLAGS) $(GLIBMM_CFLAGS) $(GLOBUS_IO_CFLAGS) $(GSOAP_CFLAGS) -I$(top_srcdir)/include
libdmcsrm_la_LIBADD   = $(LIBXML2_LIBS) $(GLIBMM_LIBS) $(top_srcdir)/src/hed/libs/loader/libarcloader.la $(top_srcdir)/src/hed/libs/message/libarcmessage.la $(top_srcdir)/src/hed/libs/data/libarcdata2.la $(top_srcdir)/src/hed/libs/common/libarccommon.la srmclient/libsrmclient.la
libdmcsrm_la_LDFLAGS  = -no-undefined -avoid-version -module
