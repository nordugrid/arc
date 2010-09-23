sbin_PROGRAMS = arexgridftpd
noinst_LTLIBRARIES = libgridftpd.la

initddir=@initddir@
if SYSV_SCRIPTS_ENABLED
GRIDFTPD_SCRIPT = a-rex-gridftpd
else
GRIDFTPD_SCRIPT =
endif
initd_SCRIPTS = $(GRIDFTPD_SCRIPT)

libgridftpd_la_SOURCES = userspec.cpp names.cpp misc.cpp
libgridftpd_la_CXXFLAGS = $(GLOBUS_FTP_CLIENT_CFLAGS) $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include

arexgridftpd_SOURCES = commands.cpp config.cpp fileroot.cpp listener.cpp \
	dataread.cpp datawrite.cpp datalist.cpp fileroot_config.cpp \
	commands.h conf.h fileroot.h misc.h names.h userspec.h

arexgridftpd_CXXFLAGS = $(GLOBUS_FTP_CLIENT_CFLAGS) $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include

arexgridftpd_LDADD = libgridftpd.la conf/libconf.la run/librun.la \
	misc/libmisc.la auth/libauth.la auth/libmap.la \
	$(GLOBUS_FTP_CONTROL_LIBS) $(GLOBUS_COMMON_LIBS) \
	$(top_srcdir)/src/hed/libs/common/libarccommon.la \
	$(top_srcdir)/src/hed/libs/globusutils/libarcglobusutils.la

arexgridftpd_LDFLAGS = -rdynamic

if GACL_ENABLED
GACL_PLUGIN = gaclplugin
else
GACL_PLUGIN =
endif

SUBDIRS = misc conf run external auth . fileplugin $(GACL_PLUGIN)

DIST_SUBDIRS = misc conf run external auth . fileplugin gaclplugin
