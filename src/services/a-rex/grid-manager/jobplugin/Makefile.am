if GACL_ENABLED
GACL_CFLAGS = -I../../../gridftpd/external/gacl
else
GACL_CFLAGS =
endif

GLOBUS_JOBPLUGIN_LIBS = $(GLOBUS_RSL_LIBS) $(GLOBUS_IO_LIBS)

pkglib_LTLIBRARIES = jobplugin.la

jobplugin_la_SOURCES = jobplugin.cpp jobplugin.h init.cpp
jobplugin_la_CXXFLAGS = $(LIBXML2_CFLAGS) $(GLIBMM_CFLAGS) $(GLOBUS_FTP_CLIENT_CFLAGS) $(GACL_CFLAGS) -I$(top_srcdir)/include
jobplugin_la_LDFLAGS = -module -avoid-version

jobplugin_la_LIBADD = \
	../../../gridftpd/fileplugin/fileplugin_la-fileplugin.lo \
	../../../gridftpd/libgridftpd_la-userspec.lo \
	../../../gridftpd/libgridftpd_la-names.lo \
	../../../gridftpd/libgridftpd_la-misc.lo \
	../../../gridftpd/auth/libmap.la \
	../misc/libmisc.la \
	../jobs/libjobs.la ../mail/libmail.la ../conf/libconf.la \
	../log/liblog.la ../files/libfiles.la ../run/librun.la \
	$(GLOBUS_JOBPLUGIN_LIBS) -lpthread
