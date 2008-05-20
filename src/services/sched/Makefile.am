pkglib_LTLIBRARIES = libgrid_sched.la
libgrid_sched_la_SOURCES = grid_sched.cpp grid_sched.h job.h job.cpp job_request.cpp job_request.h job_sched_meta.cpp job_sched_meta.h job_status.h job_status.cpp job_queue.h job_queue.cpp resource.h resource.cpp resources_handling.h resources_handling.cpp bes.cpp ibes.cpp get_factory_document.cpp
libgrid_sched_la_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
libgrid_sched_la_LIBADD   = $(top_srcdir)/src/hed/libs/loader/libarcloader.la $(top_srcdir)/src/hed/libs/message/libarcmessage.la $(top_srcdir)/src/hed/libs/security/libarcsecurity.la $(top_srcdir)/src/hed/libs/ws/libarcws.la $(top_srcdir)/src/hed/libs/common/libarccommon.la $(top_srcdir)/src/hed/libs/misc/libarcclient.la $(GLIBMM_LIBS) $(LIBXML2_LIBS) -ldb_cxx
libgrid_sched_la_LDFLAGS = -no-undefined -avoid-version -module
