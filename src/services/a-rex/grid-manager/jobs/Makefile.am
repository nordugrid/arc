noinst_LTLIBRARIES = libjobs.la

libjobs_la_SOURCES = \
	commfifo.cpp users.cpp states.cpp job.cpp job_request.cpp \
	plugins.cpp job_desc.cpp dtr_generator.cpp \
	commfifo.h   users.h   states.h   job.h   job_request.h   \
	plugins.h   job_desc.h   dtr_generator.h
libjobs_la_CXXFLAGS = -I$(top_srcdir)/include \
	$(LIBXML2_CFLAGS) $(GLIBMM_CFLAGS)
libjobs_la_LIBADD = \
	$(top_srcdir)/src/libs/data-staging/libdatastaging.la \
	$(top_srcdir)/src/hed/libs/client/libarcclient.la \
	$(top_srcdir)/src/hed/libs/common/libarccommon.la
