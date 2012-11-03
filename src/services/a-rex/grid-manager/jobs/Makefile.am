noinst_LTLIBRARIES = libjobs.la libusers.la

libjobs_la_SOURCES = \
	commfifo.cpp users.cpp states.cpp job.cpp job_request.cpp \
	job_config.cpp plugins.cpp job_desc.cpp dtr_generator.cpp \
	commfifo.h   users.h   states.h   job.h   job_request.h   \
	job_config.h   plugins.h   job_desc.h   dtr_generator.h
libjobs_la_CXXFLAGS = -I$(top_srcdir)/include \
	$(LIBXML2_CFLAGS) $(GLIBMM_CFLAGS) $(DBCXX_CPPFLAGS)
libjobs_la_LIBADD = \
	$(top_builddir)/src/libs/data-staging/libarcdatastaging.la \
	$(top_builddir)/src/hed/libs/compute/libarccompute.la \
	$(top_builddir)/src/hed/libs/common/libarccommon.la \
	$(DBCXX_LIBS)

libusers_la_SOURCES = \
	users.cpp job_config.cpp job.cpp plugins.cpp commfifo.cpp \
	job_request.cpp job_desc.cpp \
	users.h   job_config.h   job.h   plugins.h   commfifo.h   \
	job_request.h   job_desc.h
libusers_la_CXXFLAGS = -I$(top_srcdir)/include \
	$(LIBXML2_CFLAGS) $(GLIBMM_CFLAGS)  $(DBCXX_CPPFLAGS)
libusers_la_LIBADD = \
	$(top_builddir)/src/hed/libs/common/libarccommon.la \
	$(DBCXX_LIBS)
