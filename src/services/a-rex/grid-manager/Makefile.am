if GRIDFTPD_SERVICE_ENABLED
JOBPLUGIN_DIR = jobplugin
else
JOBPLUGIN_DIR =
endif

SUBDIRS = jobs run conf misc log mail files loaders $(JOBPLUGIN_DIR)

noinst_LTLIBRARIES = libgridmanager.la
pkglibexec_PROGRAMS = gm-kick gm-jobs inputcheck arc-vomsac-check arc-blahp-logger
dist_pkglibexec_SCRIPTS = arc-config-check

man_MANS = arc-vomsac-check.8 arc-config-check.1 arc-blahp-logger.8 gm-jobs.8

libgridmanager_la_SOURCES = GridManager.cpp GridManager.h
libgridmanager_la_CXXFLAGS = -I$(top_srcdir)/include \
	$(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) $(DBCXX_CPPFLAGS) $(AM_CXXFLAGS)
libgridmanager_la_LIBADD = \
	jobs/libjobs.la conf/libconf.la log/liblog.la files/libfiles.la \
	run/librun.la misc/libmisc.la mail/libmail.la \
	$(GLIBMM_LIBS) $(LIBXML2_LIBS) $(DBCXX_LIBS) -lpthread

gm_kick_SOURCES = gm_kick.cpp
gm_kick_CXXFLAGS = -I$(top_srcdir)/include \
	$(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) $(AM_CXXFLAGS)
gm_kick_LDADD = libgridmanager.la

gm_jobs_SOURCES = gm_jobs.cpp
gm_jobs_CXXFLAGS = -I$(top_srcdir)/include \
	$(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) $(DBCXX_CPPFLAGS) $(AM_CXXFLAGS)
gm_jobs_LDADD = libgridmanager.la ../delegation/libdelegation.la

inputcheck_SOURCES = inputcheck.cpp
inputcheck_CXXFLAGS = -I$(top_srcdir)/include \
	$(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) $(AM_CXXFLAGS)
inputcheck_LDADD = libgridmanager.la ../delegation/libdelegation.la \
	$(top_builddir)/src/hed/libs/data/libarcdata.la \
	$(top_builddir)/src/hed/libs/common/libarccommon.la

arc_vomsac_check_SOURCES = arc_vomsac_check.cpp
arc_vomsac_check_CXXFLAGS = -I$(top_srcdir)/include \
	$(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) $(AM_CXXFLAGS)
arc_vomsac_check_LDADD = $(top_builddir)/src/hed/libs/credential/libarccredential.la \
	$(top_builddir)/src/hed/libs/common/libarccommon.la \
	$(GLIBMM_LIBS)

arc_blahp_logger_SOURCES = arc_blahp_logger.cpp
arc_blahp_logger_CXXFLAGS = -I$(top_srcdir)/include \
	$(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) $(AM_CXXFLAGS)
arc_blahp_logger_LDADD = $(top_builddir)/src/hed/libs/credential/libarccredential.la \
	$(top_builddir)/src/hed/libs/common/libarccommon.la \
	$(GLIBMM_LIBS)
