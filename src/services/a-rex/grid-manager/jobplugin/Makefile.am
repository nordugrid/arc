GLOBUS_JOBPLUGIN_LIBS = $(GLOBUS_RSL_LIBS) $(GLOBUS_IO_LIBS)

GACL_LOCATION = ../../../gridftpd/external/gacl-0.1.5

pkglib_LTLIBRARIES = jobplugin.la

jobplugin_la_SOURCES = jobplugin.cpp jobplugin.h init.cpp
jobplugin_la_CXXFLAGS = $(LIBXML2_CFLAGS) $(GLIBMM_CFLAGS) $(GLOBUS_FTP_CLIENT_CFLAGS) -I$(GACL_LOCATION) -I$(top_srcdir)/include
jobplugin_la_LDFLAGS = -module -avoid-version

jobplugin_la_LIBADD = ../../../gridftpd/fileplugin/fileplugin_la-fileplugin.lo \
  ../../../gridftpd/libgridftpd_la-userspec.lo ../../../gridftpd/libgridftpd_la-names.lo \
  ../../../gridftpd/libgridftpd_la-misc.lo \
  ../../../gridftpd/auth/libgaclmap.la ../misc/libmisc.la \
	../jobs/libjobs.la ../mail/libmail.la ../conf/libconf.la \
	../log/liblog.la ../files/libfiles.la ../run/librun.la \
	$(GLOBUS_JOBPLUGIN_LIBS) -lpthread
	