if GRIDFTPD_SERVICE_ENABLED
JOBPLUGIN_DIR = jobplugin
else
JOBPLUGIN_DIR =
endif

SUBDIRS = jobs run conf misc log mail files loaders $(JOBPLUGIN_DIR)

noinst_LTLIBRARIES = libgridmanager.la
pkglibexec_PROGRAMS = gm-kick gm-jobs inputcheck

libgridmanager_la_SOURCES  = grid_manager.cpp grid_manager.h
libgridmanager_la_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
libgridmanager_la_LIBADD   = jobs/libjobs.la conf/libconf.la log/liblog.la files/libfiles.la \
                             run/librun.la misc/libmisc.la mail/libmail.la \
                             $(GLIBMM_LIBS) $(LIBXML2_LIBS) -lpthread

gm_kick_SOURCES = gm_kick.cpp jobs/users.cpp
gm_kick_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
gm_kick_LDADD   = libgridmanager.la

gm_jobs_SOURCES = gm_jobs.cpp
gm_jobs_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
gm_jobs_LDADD   = libgridmanager.la

inputcheck_SOURCES = inputcheck.cpp
inputcheck_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
inputcheck_LDADD   = libgridmanager.la
