noinst_LTLIBRARIES = libjobs.la

libjobs_ladir=$(includedir)
libjobs_la_HEADERS=commfifo.h   users.h   states.h   job.h   job_request.h   plugins.h
libjobs_la_SOURCES=commfifo.cpp users.cpp states.cpp job.cpp job_request.cpp plugins.cpp
libjobs_la_CXXFLAGS=$(LIBXML2_CFLAGS) $(GLIBMM_CFLAGS) $(GLOBUS_RSL_CFLAGS)
libjobs_la_LIBADD=$(top_srcdir)/src/libs/common/libarccommon.la
#../mail/libmail.la ../run/librun.la ../files/libfiles.la ../url/liburl.la ../jobdesc/libjobdesc.la ../conf/libconf.la ../log/liblog.la ../misc/libmisc.la 


