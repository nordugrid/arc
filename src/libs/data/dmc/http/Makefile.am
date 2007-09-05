pkglib_LTLIBRARIES = libdmchttp.la

libdmchttp_la_SOURCES  = DMCHTTP.cpp DataPointHTTP.cpp \
                         DMCHTTP.h   DataPointHTTP.h
libdmchttp_la_CPPFLAGS = ${LIBXML2_CFLAGS} ${GLIBMM_CFLAGS} -I$(top_srcdir)/src/libs -I$(top_srcdir)/src/hed/libs
libdmchttp_la_LIBADD   = ${LIBXML2_LIBS} ${GLIBMM_LIBS}
libdmchttp_la_LDFLAGS  = -no-undefined -avoid-version -module
