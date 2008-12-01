pkglib_LTLIBRARIES = libmccgsi.la

libmccgsi_la_SOURCES  = MCCGSI.cpp PayloadGSIStream.cpp \
                        MCCGSI.h   PayloadGSIStream.h

libmccgsi_la_CXXFLAGS = -I$(top_srcdir)/include \
                        $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) \
                        $(GLOBUS_GSSAPI_GSI_CFLAGS)

libmccgsi_la_LIBADD   = $(top_srcdir)/src/hed/libs/common/libarccommon.la \
                        $(top_srcdir)/src/hed/libs/loader/libarcloader.la \
                        $(top_srcdir)/src/hed/libs/message/libarcmessage.la \
                        $(top_srcdir)/src/hed/libs/globusutils/libarcglobusutils.la \
                        $(GLIBMM_LIBS) $(LIBXML2_LIBS) \
                        $(GLOBUS_GSSAPI_GSI_LIBS) 

libmccgsi_la_LDFLAGS  = -no-undefined -avoid-version -module
