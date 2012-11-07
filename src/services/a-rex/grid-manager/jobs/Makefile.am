noinst_LTLIBRARIES = libjobs.la

libjobs_la_SOURCES = \
	commfifo.cpp states.cpp job.cpp job_request.cpp \
	plugins.cpp dtr_generator.cpp \
	commfifo.h  states.h   job.h   job_request.h   \
	plugins.h   dtr_generator.h
libjobs_la_CXXFLAGS = -I$(top_srcdir)/include \
	$(LIBXML2_CFLAGS) $(GLIBMM_CFLAGS) $(DBCXX_CPPFLAGS)
libjobs_la_LIBADD = \
	$(top_builddir)/src/libs/data-staging/libarcdatastaging.la \
	$(top_builddir)/src/hed/libs/compute/libarccompute.la \
	$(top_builddir)/src/hed/libs/common/libarccommon.la \
	$(DBCXX_LIBS)
