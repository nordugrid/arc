noinst_LTLIBRARIES = libjobs.la

libjobs_la_SOURCES=commfifo.cpp users.cpp states.cpp job.cpp job_request.cpp plugins.cpp job_desc.cpp \
                   commfifo.h   users.h   states.h   job.h   job_request.h   plugins.h   job_desc.h
libjobs_la_CXXFLAGS=$(LIBXML2_CFLAGS) $(GLIBMM_CFLAGS) -I$(top_srcdir)/include
libjobs_la_LIBADD=$(top_srcdir)/src/hed/libs/common/libarccommon.la $(top_srcdir)/src/hed/libs/client/libarcclient.la
#../mail/libmail.la ../run/librun.la ../files/libfiles.la ../url/liburl.la ../jobdesc/libjobdesc.la ../conf/libconf.la ../log/liblog.la ../misc/libmisc.la 


