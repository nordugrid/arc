lib_LTLIBRARIES = libarcws.la
libarcws_ladir = $(pkgincludedir)
nodist_EXTRA_libarcws_la_SOURCES = dummy.cpp
libarcws_la_SOURCES =
libarcws_la_LIBADD = $(top_srcdir)/src/hed/libs/infosys/libinfosys.la \
                     $(top_srcdir)/src/hed/libs/ws-security/libwssecurity.la
# libwsrf is included through libinfosys
# libwsaddressing is included through libwsrf
# stupid libtool
