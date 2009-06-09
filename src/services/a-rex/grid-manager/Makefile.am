SUBDIRS = jobs run conf misc log mail files loaders

noinst_LTLIBRARIES = libgridmanager.la
pkglibexec_PROGRAMS = gm-kick

libgridmanager_la_SOURCES  = grid_manager.cpp grid_manager.h
libgridmanager_la_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
libgridmanager_la_LIBADD   = jobs/libjobs.la conf/libconf.la log/liblog.la files/libfiles.la \
                             run/librun.la misc/libmisc.la mail/libmail.la \
                             $(GLIBMM_LIBS) $(LIBXML2_LIBS) -lpthread

gm_kick_SOURCES = gm_kick.cpp jobs/users.cpp
gm_kick_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
gm_kick_LDADD   = libgridmanager.la
