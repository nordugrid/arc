GLOBUS_FILEPLUGIN_LIBS = $(GLOBUS_RSL_LIBS) $(GLOBUS_IO_LIBS)

pkglib_LTLIBRARIES = fileplugin.la

fileplugin_la_SOURCES = init.cpp fileplugin.cpp fileplugin.h
fileplugin_la_CXXFLAGS = $(GLOBUS_IO_CFLAGS) $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
fileplugin_la_LDFLAGS = -module -avoid-version
fileplugin_la_LIBADD  = ../libgridftpd_la-misc.lo ../libgridftpd_la-userspec.lo ../libgridftpd_la-names.lo \
  ../auth/libmap.la \
  $(GLOBUS_FILEPLUGIN_LIBS) \
  $(top_srcdir)/src/hed/libs/common/libarccommon.la

