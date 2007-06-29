SUBDIRS=jobs cache run conf misc url jobdesc files log mail

noinst_PROGRAMS=grid-manager
SOURCES =

#grid_manager_HEADERS=
grid_manager_SOURCES=main.cpp
grid_manager_CPPFLAGS=$(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/src/libs/
grid_manager_LDADD=jobs/libjobs.la cache/libcache.la run/librun.la jobdesc/libjobdesc.la conf/libconf.la log/liblog.la files/libfiles.la misc/libmisc.la url/liburl.la mail/libmail.la jobdesc/jsdl/libnamespace.la
grid_manager_LDFLAGS=$(GLIBMM_LIBS) $(LIBXML2_LIBS)
