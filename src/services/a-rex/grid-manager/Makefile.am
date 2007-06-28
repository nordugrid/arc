SUBDIRS=jobs cache run conf misc url files jobdesc log mail

noinst_PROGRAMS=grid-manager
SOURCES =

#grid_manager_HEADERS=
grid_manager_SOURCES=main.cpp
grid_manager_CPPFLAGS=$(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/src/libs/
grid_manager_LDADD=jobs/libjobs.la cache/libcache.la run/librun.la conf/libconf.la files/libfiles.la misc/libmisc.la url/liburl.la jobdesc/libjobdesc.la log/liblog.la mail/libmail.la
grid_manager_LDFLAGS=$(GLIBMM_LIBS) $(LIBXML2_LIBS) $(GSOAP_LIBS)
