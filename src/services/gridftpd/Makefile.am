sbin_PROGRAMS = gridftpd
noinst_LTLIBRARIES = libgridftpd.la

if SYSV_SCRIPTS_ENABLED
GRIDFTPD_SCRIPT = gridftpd.init
else
GRIDFTPD_SCRIPT =
endif
EXTRA_DIST = $(GRIDFTPD_SCRIPT)

if SYSV_SCRIPTS_ENABLED
# Not using initd_SCRIPTS due to rename.
install-data-local:
	$(MKDIR_P) "$(DESTDIR)$(initddir)"
	$(INSTALL_SCRIPT) gridftpd.init $(DESTDIR)$(initddir)/gridftpd
uninstall-local:
	rm -f $(DESTDIR)$(initddir)/gridftpd
endif

libgridftpd_la_SOURCES = userspec.cpp names.cpp misc.cpp
libgridftpd_la_CXXFLAGS = -I$(top_srcdir)/include \
	$(GLOBUS_FTP_CLIENT_CFLAGS) $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS)

gridftpd_SOURCES = commands.cpp config.cpp fileroot.cpp listener.cpp \
	dataread.cpp datawrite.cpp datalist.cpp fileroot_config.cpp \
	commands.h conf.h fileroot.h misc.h names.h userspec.h

gridftpd_CXXFLAGS = -I$(top_srcdir)/include \
	$(GLOBUS_FTP_CLIENT_CFLAGS) $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS)

gridftpd_LDADD = libgridftpd.la conf/libconf.la run/librun.la \
	misc/libmisc.la auth/libauth.la auth/libmap.la \
	$(top_builddir)/src/hed/libs/globusutils/libarcglobusutils.la \
	$(top_builddir)/src/hed/libs/common/libarccommon.la \
	$(GLOBUS_FTP_CONTROL_LIBS) $(GLOBUS_COMMON_LIBS)

gridftpd_LDFLAGS = -rdynamic

if GACL_ENABLED
GACL_PLUGIN = gaclplugin
else
GACL_PLUGIN =
endif

SUBDIRS = misc conf run external auth . fileplugin $(GACL_PLUGIN)

DIST_SUBDIRS = misc conf run external auth . fileplugin gaclplugin
