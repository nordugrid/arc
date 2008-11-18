pkglib_LTLIBRARIES = libslcs.la
libslcs_la_SOURCES  = slcs.cpp slcs.h
libslcs_la_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
libslcs_la_LIBADD   = $(top_srcdir)/src/hed/libs/common/libarccommon.la \
                    $(top_srcdir)/src/hed/libs/loader/libarcloader.la \
                    $(top_srcdir)/src/hed/libs/message/libarcmessage.la \
                    $(top_srcdir)/src/hed/libs/credential/libarccredential.la \
                    $(GLIBMM_LIBS) $(LIBXML2_LIBS)
libslcs_la_LDFLAGS  = -no-undefined -avoid-version -module
