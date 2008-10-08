SUBDIRS = jobs run conf misc log mail jobdesc files loaders

noinst_LTLIBRARIES = libgridmanager.la
pkglibexec_PROGRAMS = gm-kick

libgridmanager_la_SOURCES  = grid_manager.cpp grid_manager.h
libgridmanager_la_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) $(GLOBUS_RSL_CFLAGS) -I$(top_srcdir)/include
libgridmanager_la_LIBADD   = jobs/libjobs.la conf/libconf.la log/liblog.la files/libfiles.la \
                             jobdesc/libjobdesc.la run/librun.la misc/libmisc.la mail/libmail.la \
                             $(GLIBMM_LIBS) $(LIBXML2_LIBS) $(GLOBUS_RSL_LIBS) -lpthread

gm_kick_SOURCES = gm_kick.cpp jobs/users.cpp
gm_kick_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) $(GLOBUS_RSL_CFLAGS) -I$(top_srcdir)/include
gm_kick_LDADD   = libgridmanager.la
