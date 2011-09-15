GLOBUS_FILEPLUGIN_LIBS = $(GLOBUS_RSL_LIBS) $(GLOBUS_IO_LIBS)

pkglib_LTLIBRARIES = fileplugin.la

fileplugin_la_SOURCES = init.cpp fileplugin.cpp fileplugin.h
fileplugin_la_CXXFLAGS = -I$(top_srcdir)/include \
	$(GLOBUS_IO_CFLAGS) $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS)
fileplugin_la_LDFLAGS = -no-undefined -avoid-version -module
fileplugin_la_LIBADD = \
	../libgridftpd_la-misc.lo ../libgridftpd_la-userspec.lo \
	../libgridftpd_la-names.lo ../auth/libmap.la \
	$(top_builddir)/src/hed/libs/common/libarccommon.la \
	$(GLOBUS_FILEPLUGIN_LIBS)
