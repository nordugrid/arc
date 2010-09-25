pkglib_LTLIBRARIES = gaclplugin.la

GACL_LOCATION = ../external/gacl

gaclplugin_la_SOURCES = init.cpp util.cpp util.h util++.cpp util++.h gaclplugin.cpp gaclplugin.h
gaclplugin_la_CXXFLAGS = $(GLOBUS_IO_CFLAGS) $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(GACL_LOCATION) -I$(top_srcdir)/include
gaclplugin_la_LDFLAGS = -module -avoid-version

gaclplugin_la_LIBADD = ../libgridftpd_la-misc.lo ../libgridftpd_la-userspec.lo ../libgridftpd_la-names.lo \
	../auth/libgaclmap.la ../auth/libauth.la $(GLOBUS_IO_LIBS) \
	$(top_srcdir)/src/hed/libs/common/libarccommon.la
