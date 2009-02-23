pkglib_LTLIBRARIES = libdmcarc.la

libdmcarc_la_SOURCES  = DMCARC.cpp DataPointARC.cpp \
                         DMCARC.h   DataPointARC.h
libdmcarc_la_CXXFLAGS = $(LIBXML2_CFLAGS) $(GLIBMM_CFLAGS) -I$(top_srcdir)/include
libdmcarc_la_LIBADD   = $(LIBXML2_LIBS) $(GLIBMM_LIBS) $(top_srcdir)/src/hed/libs/loader/libarcloader.la $(top_srcdir)/src/hed/libs/message/libarcmessage.la $(top_srcdir)/src/hed/libs/data/libarcdata2.la $(top_srcdir)/src/hed/libs/common/libarccommon.la $(top_srcdir)/src/hed/libs/client/libarcclient.la
libdmcarc_la_LDFLAGS  = -no-undefined -avoid-version -module
