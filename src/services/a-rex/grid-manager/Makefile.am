SUBDIRS=jobs cache run conf misc log mail jobdesc files loaders

sbin_PROGRAMS=grid-manager

grid_manager_SOURCES=main.cpp
grid_manager_CXXFLAGS=$(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) $(GLOBUS_RSL_CFLAGS) -I$(top_srcdir)/include
grid_manager_LDADD=jobs/libjobs.la cache/libcache.la conf/libconf.la log/liblog.la files/libfiles.la jobdesc/libjobdesc.la run/librun.la misc/libmisc.la mail/libmail.la 
#grid_manager_LDADD=jobs/libjobs.la run/librun.la conf/libconf.la files/libfiles.la cache/libcache.la
grid_manager_LDFLAGS=$(GLIBMM_LIBS) $(LIBXML2_LIBS) $(GLOBUS_RSL_LIBS)
