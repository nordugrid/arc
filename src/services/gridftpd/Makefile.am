sbin_PROGRAMS = a-rex-gridftpd-bin
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

a_rex_gridftpd_bin_SOURCES = commands.cpp config.cpp fileroot.cpp listener.cpp \
	dataread.cpp datawrite.cpp datalist.cpp fileroot_config.cpp \
	commands.h conf.h fileroot.h misc.h names.h userspec.h

a_rex_gridftpd_bin_CXXFLAGS = $(GLOBUS_FTP_CLIENT_CFLAGS) $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include

a_rex_gridftpd_bin_LDADD = libgridftpd.la conf/libconf.la run/librun.la \
  misc/libmisc.la auth/libauth.la auth/libmap.la \
	$(GLOBUS_FTP_CLIENT_LIBS) $(GLOBUS_COMMON_LIBS) \
	$(top_srcdir)/src/hed/libs/common/libarccommon.la \
	$(top_srcdir)/src/hed/libs/globusutils/libarcglobusutils.la

a_rex_gridftpd_bin_LDFLAGS = -rdynamic

if XMLSEC_ENABLED
SUBDIRS = misc conf run external auth . fileplugin gaclplugin
else
SUBDIRS = misc conf run auth . fileplugin
endif
