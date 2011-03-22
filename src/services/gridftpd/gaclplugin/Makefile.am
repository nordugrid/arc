pkglib_LTLIBRARIES = gaclplugin.la

GACL_LOCATION = ../external/gacl

gaclplugin_la_SOURCES = init.cpp util.cpp util.h util++.cpp util++.h \
	gaclplugin.cpp gaclplugin.h
gaclplugin_la_CXXFLAGS = -I$(top_srcdir)/include \
	$(GLOBUS_IO_CFLAGS) -I$(GACL_LOCATION) \
	$(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS)
gaclplugin_la_LDFLAGS = -no-undefined -avoid-version -module

gaclplugin_la_LIBADD = ../libgridftpd_la-misc.lo \
	../libgridftpd_la-userspec.lo ../libgridftpd_la-names.lo \
	../auth/libmap.la \
	$(top_srcdir)/src/hed/libs/common/libarccommon.la \
	$(GLOBUS_IO_LIBS)
