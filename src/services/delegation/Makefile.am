pkglib_LTLIBRARIES = libdelegation.la
libdelegation_la_SOURCES  = delegation.cpp delegation.h
libdelegation_la_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
libdelegation_la_LIBADD   = $(top_srcdir)/src/hed/libs/common/libarccommon.la \
                    $(top_srcdir)/src/hed/libs/loader/libarcloader.la \
                    $(top_srcdir)/src/hed/libs/message/libarcmessage.la \
                    $(top_srcdir)/src/hed/libs/delegation/libarcdelegation.la \
                    $(GLIBMM_LIBS) $(LIBXML2_LIBS)
libdelegation_la_LDFLAGS  = -no-undefined -avoid-version -module
