pkglib_LTLIBRARIES = libdmchttp.la

libdmchttp_la_SOURCES  = DMCHTTP.cpp DataPointHTTP.cpp \
                         DMCHTTP.h   DataPointHTTP.h
libdmchttp_la_CXXFLAGS = $(LIBXML2_CFLAGS) $(GLIBMM_CFLAGS) -I$(top_srcdir)/include
libdmchttp_la_LIBADD   = $(LIBXML2_LIBS) $(GLIBMM_LIBS) $(top_srcdir)/src/hed/libs/loader/libarcloader.la  $(top_srcdir)/src/hed/libs/message/libmessage.la $(top_srcdir)/src/hed/libs/data/libarcdata2.la
libdmchttp_la_LDFLAGS  = -no-undefined -avoid-version -module
