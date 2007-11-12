SUBDIRS=jobs cache run conf misc log mail jobdesc files 

sbin_PROGRAMS=grid-manager

grid_manager_SOURCES=main.cpp
grid_manager_CXXFLAGS=$(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) $(GLOBUS_RSL_CFLAGS) -I$(top_srcdir)/include
grid_manager_LDADD=jobs/libjobs.la cache/libcache.la run/librun.la conf/libconf.la log/liblog.la files/libfiles.la jobdesc/libjobdesc.la misc/libmisc.la mail/libmail.la jobdesc/jsdl/libnamespace.la
#grid_manager_LDADD=jobs/libjobs.la run/librun.la conf/libconf.la files/libfiles.la cache/libcache.la
grid_manager_LDFLAGS=$(GLIBMM_LIBS) $(LIBXML2_LIBS) $(GLOBUS_RSL_LIBS)
